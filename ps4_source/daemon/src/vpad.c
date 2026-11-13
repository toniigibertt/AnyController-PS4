/*
 * Virtual DualShock 4 Device Handler for OneFamily Daemon
 * Creates native Virtual DualShock 4 (Device Type 1) via libScePad
 * Compatible with XMB, PS4 games, homebrew, and system overlays
 * Supports Multi-User & Player Slot binding via libSceUserService & libSceMbus
 */

#include "vpad.h"
#include "bt_driver.h"
#include "usb_driver.h"
#include "config.h"
#include "log.h"
#include "daemon_syscalls.h"
#include "ds4.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    uint32_t size;
    uint32_t type;
    uint32_t flags;
    uint32_t reserved;
} ScePadVirtualDeviceParam;

// libScePad APIs
typedef int (*scePadInit_t)(void);
typedef int (*scePadMbusInit_t)(void);
typedef int (*scePadSetProcessPrivilege_t)(int);
typedef int (*scePadSetConnection_t)(int handle, int connection);
typedef int (*scePadVirtualDeviceAddDevice_t)(const ScePadVirtualDeviceParam *param, int *out_handle);
typedef int (*scePadVirtualDeviceInsertData_t)(int handle, const void *data);
typedef int (*scePadVirtualDeviceDeleteDevice_t)(int handle);
typedef int (*scePadVirtualDeviceGetRemoteSetting_t)(int handle, void *out_setting);
typedef int32_t (*scePadGetHandle_t)(int32_t userId, uint32_t type, uint32_t index);
typedef int32_t (*scePadOpen_t)(int32_t userId, int32_t type, int32_t index, void *param);
typedef int32_t (*scePadClose_t)(int32_t handle);
typedef int32_t (*scePadReadState_t)(int32_t handle, void *pData);
typedef int32_t (*scePadGetControllerInformation_t)(int32_t handle, void *pInfo);
typedef int32_t (*scePadGetDeviceId_t)(int32_t handle, int64_t *outDeviceId);
typedef int32_t (*scePadDisconnectPort_t)(int32_t handle);
typedef int32_t (*scePadDisconnectDevice_t)(int32_t handle_or_devId);
typedef int32_t (*scePadSetLightBar_t)(int32_t handle, const OrbisPadColor *pColor);
typedef int32_t (*scePadResetLightBar_t)(int32_t handle);
typedef int32_t (*scePadSetLightBarBaseBrightness_t)(int32_t handle, int32_t brightness);

// libSceMbus APIs
typedef int (*sceMbusInit_t)(void);
typedef int (*sceMbusBindDeviceWithUserId_t)(int deviceId, int32_t userId);
typedef int (*sceMbusDisconnectDevice_t)(int deviceId);
typedef int32_t (*sceMbusResolveByHandle_t)(int32_t handle, int64_t *outDeviceId);
typedef int32_t (*sceMbusUserLogin_t)(int32_t userId);
typedef int32_t (*sceMbusUserLogout_t)(int32_t userId);

// libSceHidControl APIs
typedef int32_t (*sceHidControlInit_t)(void);
typedef int32_t (*sceHidControlGetDeviceId_t)(int32_t handle, int64_t *outDeviceId);
typedef int32_t (*sceHidControlDisconnectPort_t)(int32_t handle);
typedef int32_t (*sceHidControlDisconnectDevice_t)(int32_t deviceId);
typedef int32_t (*sceHidControlDisconnectHandle_t)(int32_t handle);

// libSceUserService APIs
#define SCE_USER_SERVICE_MAX_LOGIN_USERS 4
#define SCE_USER_SERVICE_MAX_REGISTER_USERS 16
#define SCE_USER_SERVICE_USER_ID_INVALID (-1)

typedef int32_t SceUserServiceUserId;

typedef struct {
    SceUserServiceUserId userId[SCE_USER_SERVICE_MAX_LOGIN_USERS];
} SceUserServiceLoginUserIdList;

typedef struct {
    SceUserServiceUserId userId[SCE_USER_SERVICE_MAX_REGISTER_USERS];
} SceUserServiceRegisteredUserIdList;

typedef int (*sceUserServiceInitialize_t)(void *params);
typedef int (*sceUserServiceGetInitialUser_t)(SceUserServiceUserId *userId);
typedef int (*sceUserServiceGetLoginUserIdList_t)(SceUserServiceLoginUserIdList *userIdList);
typedef int (*sceUserServiceGetRegisteredUserIdList_t)(SceUserServiceRegisteredUserIdList *userIdList);
typedef int (*sceUserServiceGetUserName_t)(SceUserServiceUserId userId, char *userName, size_t size);
typedef int (*sceUserServiceLogin_t)(SceUserServiceUserId userId);
typedef int (*sceUserServiceLogout_t)(SceUserServiceUserId userId);
typedef int (*sceUserServiceSetLightBarBaseBrightness_t)(SceUserServiceUserId userId, int32_t brightness);

static int g_hMbus = 0;
static int g_hPad = 0;
static int g_hUserService = 0;
static int g_hHidControl = 0;
static int g_vpad_handle = -1;
static bool g_vpad_active = false;
static vpad_rumble_cb_t g_rumble_cb = NULL;

typedef struct {
    int handle;
    bool active;
    VPadSource source;
    uint64_t last_send_time_us;
    OrbisPadData last_sent_data;
    bool has_sent_first;
    int last_ps_state;
    uint32_t last_logged_buttons;
    int logged_insert_packets;
    uint8_t last_rumble_lg;
    uint8_t last_rumble_sm;
    uint8_t last_raw_rumble_lg;
    uint8_t last_raw_rumble_sm;
    uint64_t last_rumble_change_us;
    uint8_t last_color_r;
    uint8_t last_color_g;
    uint8_t last_color_b;
    uint64_t last_rumble_send_us;
    int stop_retries_remaining;
    int assigned_slot;
    char assigned_username[64];
    uint8_t packet_counter;
    uint64_t device_key;
    uint64_t ps_pulse_start_us;
    uint64_t ps_pulse_until_us;
    bool ps_pulse_active;
    uint64_t ps_followup_at_us;
    bool ps_followup_pending;
    bool user_interacted;
    int slot_open_login_count;
    uint64_t last_remote_poll_us;
} VPadSlotState;

static VPadSlotState g_slots[VPAD_MAX_SLOTS];
static int32_t s_ds4_handles[SCE_USER_SERVICE_MAX_LOGIN_USERS] = { -1, -1, -1, -1 };
static SceUserServiceUserId s_ds4_uids[SCE_USER_SERVICE_MAX_LOGIN_USERS] = { -1, -1, -1, -1 };

static scePadInit_t                          p_scePadInit = NULL;
static scePadMbusInit_t                      p_scePadMbusInit = NULL;
static scePadSetProcessPrivilege_t           p_scePadSetProcessPrivilege = NULL;
static scePadSetConnection_t                 p_scePadSetConnection = NULL;
static scePadVirtualDeviceAddDevice_t        p_scePadVirtualDeviceAddDevice = NULL;
static scePadVirtualDeviceInsertData_t       p_scePadVirtualDeviceInsertData = NULL;
static scePadVirtualDeviceDeleteDevice_t     p_scePadVirtualDeviceDeleteDevice = NULL;
static scePadVirtualDeviceGetRemoteSetting_t p_scePadVirtualDeviceGetRemoteSetting = NULL;
static scePadGetHandle_t                     p_scePadGetHandle = NULL;
static scePadOpen_t                          p_scePadOpen = NULL;
static scePadClose_t                         p_scePadClose = NULL;
static scePadReadState_t                     p_scePadReadState = NULL;
static scePadGetControllerInformation_t      p_scePadGetControllerInformation = NULL;
static scePadGetDeviceId_t                   p_scePadGetDeviceId = NULL;
static scePadDisconnectPort_t                p_scePadDisconnectPort = NULL;
static scePadDisconnectDevice_t              p_scePadDisconnectDevice = NULL;
static scePadSetLightBar_t                   p_scePadSetLightBar = NULL;
static scePadResetLightBar_t                 p_scePadResetLightBar = NULL;
static scePadSetLightBarBaseBrightness_t     p_scePadSetLightBarBaseBrightness = NULL;

static int vpad_ensure_subsystem(void);

static sceMbusInit_t                         p_sceMbusInit = NULL;
static sceMbusBindDeviceWithUserId_t         p_sceMbusBindDeviceWithUserId = NULL;
static sceMbusDisconnectDevice_t             p_sceMbusDisconnectDevice = NULL;
static sceMbusResolveByHandle_t              p_sceMbusResolveByHandle = NULL;
static sceMbusUserLogin_t                    p_sceMbusUserLogin = NULL;
static sceMbusUserLogout_t                   p_sceMbusUserLogout = NULL;

static sceHidControlInit_t                   p_sceHidControlInit = NULL;
static sceHidControlGetDeviceId_t            p_sceHidControlGetDeviceId = NULL;
static sceHidControlDisconnectPort_t         p_sceHidControlDisconnectPort = NULL;
static sceHidControlDisconnectDevice_t       p_sceHidControlDisconnectDevice = NULL;
static sceHidControlDisconnectHandle_t       p_sceHidControlDisconnectHandle = NULL;

static sceUserServiceInitialize_t                  p_sceUserServiceInitialize = NULL;
static sceUserServiceGetInitialUser_t              p_sceUserServiceGetInitialUser = NULL;
static sceUserServiceGetLoginUserIdList_t          p_sceUserServiceGetLoginUserIdList = NULL;
static sceUserServiceGetRegisteredUserIdList_t     p_sceUserServiceGetRegisteredUserIdList = NULL;
static sceUserServiceGetUserName_t                 p_sceUserServiceGetUserName = NULL;
static sceUserServiceLogin_t                       p_sceUserServiceLogin = NULL;
static sceUserServiceLogout_t                      p_sceUserServiceLogout = NULL;
static sceUserServiceSetLightBarBaseBrightness_t   p_sceUserServiceSetLightBarBaseBrightness = NULL;

static int s_assigned_slot = 1;
static char s_assigned_username[64] = "Player 1";
static int s_last_login_count = -1;

static bool is_our_virtual_handle(int32_t h);

static void vpad_close_cached_ds4_handles(void) {
    for (int i = 0; i < SCE_USER_SERVICE_MAX_LOGIN_USERS; i++) {
        if (s_ds4_handles[i] >= 0) {
            if (p_scePadClose && !is_our_virtual_handle(s_ds4_handles[i])) {
                p_scePadClose(s_ds4_handles[i]);
            }
            s_ds4_handles[i] = -1;
            s_ds4_uids[i] = SCE_USER_SERVICE_USER_ID_INVALID;
        }
    }
}

struct ElevateArgs {
    uint64_t target_authid;
    uint64_t original_authid;
    uint8_t target_caps[16];
    uint8_t original_caps[16];
    int restore;
    int rc;
};

static int kernel_elevate_ucred(void* thread, struct ElevateArgs* args) {
    if (!thread || !args) return 22;
    uint8_t* td = (uint8_t*)thread;
    uint8_t* proc = *(uint8_t**)(td + 8); // td->td_proc
    if (!proc || ((uint64_t)proc >> 48) != 0xFFFF) return 2;

    // Primary across Orbis OS firmwares (verified via libjbc): proc->p_ucred at proc + 0x40
    uint8_t* ucred = *(uint8_t**)(proc + 0x40);
    if (!ucred || ((uint64_t)ucred >> 48) != 0xFFFF) {
        ucred = *(uint8_t**)(td + 0x130);
    }
    if (!ucred || ((uint64_t)ucred >> 48) != 0xFFFF) {
        ucred = *(uint8_t**)(td + 0x138);
    }
    if (!ucred || ((uint64_t)ucred >> 48) != 0xFFFF) {
        ucred = *(uint8_t**)(td + 0x140);
    }
    if (!ucred || ((uint64_t)ucred >> 48) != 0xFFFF) {
        ucred = *(uint8_t**)(proc + 0x140);
    }
    if (!ucred || ((uint64_t)ucred >> 48) != 0xFFFF) return 3;

    uint64_t* authid_ptr = (uint64_t*)(ucred + 0x58);
    uint64_t* caps_ptr   = (uint64_t*)(ucred + 0x60);
    uint8_t*  attr_ptr   = (uint8_t*)(ucred + 0x83);

    if (args->restore) {
        if (args->original_authid != 0) {
            *authid_ptr = args->original_authid;
            memcpy(caps_ptr, args->original_caps, 16);
        }
    } else {
        args->original_authid = *authid_ptr;
        memcpy(args->original_caps, caps_ptr, 16);
        if (args->target_authid != 0) {
            *authid_ptr = args->target_authid;
        }
        caps_ptr[0] = 0xFFFFFFFFFFFFFFFFULL;
        caps_ptr[1] = 0xFFFFFFFFFFFFFFFFULL;
        *attr_ptr |= 0x80;
    }
    args->rc = 0;
    return 0;
}

static int vpad_elevate_creds(int restore) {
    static uint64_t s_orig_authid = 0;
    static uint8_t s_orig_caps[16];
    static bool s_is_elevated = false;

    if (restore) {
        if (!s_is_elevated) return 0;
        struct ElevateArgs args;
        memset(&args, 0, sizeof(args));
        args.restore = 1;
        args.original_authid = s_orig_authid;
        memcpy(args.original_caps, s_orig_caps, 16);
        long ret = daemon_syscall(11 /* SYS_KEXEC */, (long)kernel_elevate_ucred, (long)&args, 0);
        if (ret == 0 && args.rc == 0) {
            s_is_elevated = false;
            daemon_log(LOG_LEVEL_INFO, "vpad: Original credentials restored (authid=0x%llX)", (unsigned long long)s_orig_authid);
            return 0;
        }
        return -1;
    }

    if (s_is_elevated) return 0;

    struct ElevateArgs args;
    memset(&args, 0, sizeof(args));
    args.target_authid = 0x3800000000010003ULL; // SceShellCore system service authority
    args.restore = 0;
    long ret = daemon_syscall(11 /* SYS_KEXEC */, (long)kernel_elevate_ucred, (long)&args, 0);
    if (ret == 0 && args.rc == 0) {
        s_orig_authid = args.original_authid;
        memcpy(s_orig_caps, args.original_caps, 16);
        s_is_elevated = true;
        daemon_log(LOG_LEVEL_INFO, "vpad: Credentials elevated to system service (orig_authid=0x%llX -> 0x%llX)",
                   (unsigned long long)s_orig_authid, (unsigned long long)args.target_authid);
        return 0;
    }
    daemon_log(LOG_LEVEL_WARN, "vpad: Credential elevation returned ret=%ld rc=%d", ret, args.rc);
    return -1;
}

static inline int abs_diff(int a, int b) {
    int d = a - b;
    return d < 0 ? -d : d;
}

static int load_prx(const char *name, int *out_h) {
    char path[256];
    int h = 0;

    // 1. Try full system library path
    snprintf(path, sizeof(path), "/system/common/lib/%s", name);
    if (daemon_sys_load_prx(path, &h) >= 0 && h != 0) {
        *out_h = h;
        return 0;
    }

    // 2. Try raw library name
    if (daemon_sys_load_prx(name, &h) >= 0 && h != 0) {
        *out_h = h;
        return 0;
    }

    return -1;
}

static void init_user_service(void) {
    if (g_hUserService == 0) {
        if (load_prx("libSceUserService.sprx", &g_hUserService) == 0 && g_hUserService != 0) {
            daemon_log(LOG_LEVEL_INFO, "libSceUserService.sprx loaded successfully (handle=0x%X)", g_hUserService);
            daemon_sys_dlsym(g_hUserService, "sceUserServiceInitialize", (void**)&p_sceUserServiceInitialize);
            daemon_sys_dlsym(g_hUserService, "sceUserServiceGetInitialUser", (void**)&p_sceUserServiceGetInitialUser);
            daemon_sys_dlsym(g_hUserService, "sceUserServiceGetLoginUserIdList", (void**)&p_sceUserServiceGetLoginUserIdList);
            daemon_sys_dlsym(g_hUserService, "sceUserServiceGetRegisteredUserIdList", (void**)&p_sceUserServiceGetRegisteredUserIdList);
            daemon_sys_dlsym(g_hUserService, "sceUserServiceGetUserName", (void**)&p_sceUserServiceGetUserName);
            daemon_sys_dlsym(g_hUserService, "sceUserServiceLogin", (void**)&p_sceUserServiceLogin);
            daemon_sys_dlsym(g_hUserService, "sceUserServiceLogout", (void**)&p_sceUserServiceLogout);
            daemon_sys_dlsym(g_hUserService, "sceUserServiceSetLightBarBaseBrightness", (void**)&p_sceUserServiceSetLightBarBaseBrightness);
            if (p_sceUserServiceInitialize) {
                int r = p_sceUserServiceInitialize(NULL);
                daemon_log(LOG_LEVEL_INFO, "sceUserServiceInitialize() = 0x%08X", r);
            }
        } else {
            daemon_log(LOG_LEVEL_WARN, "Failed to load libSceUserService.sprx");
        }
    }
}

static void init_hid_control(void) {
    if (g_hHidControl == 0) {
        if (load_prx("libSceHidControl.sprx", &g_hHidControl) == 0 && g_hHidControl != 0) {
            daemon_log(LOG_LEVEL_INFO, "libSceHidControl.sprx loaded successfully (handle=0x%X)", g_hHidControl);
            daemon_sys_dlsym(g_hHidControl, "sceHidControlInit", (void**)&p_sceHidControlInit);
            daemon_sys_dlsym(g_hHidControl, "sceHidControlGetDeviceId", (void**)&p_sceHidControlGetDeviceId);
            daemon_sys_dlsym(g_hHidControl, "sceHidControlDisconnectPort", (void**)&p_sceHidControlDisconnectPort);
            daemon_sys_dlsym(g_hHidControl, "sceHidControlDisconnectDevice", (void**)&p_sceHidControlDisconnectDevice);
            daemon_sys_dlsym(g_hHidControl, "sceHidControlDisconnectHandle", (void**)&p_sceHidControlDisconnectHandle);
            if (p_sceHidControlInit) {
                int r = p_sceHidControlInit();
                daemon_log(LOG_LEVEL_INFO, "sceHidControlInit() = 0x%08X", r);
            }
        }
    }
}

static inline bool is_valid_dev_id(int64_t id) {
    return (id > 0 && (uint32_t)id != 0xFFFFFFFFU);
}

static bool is_our_virtual_handle(int32_t h) {
    if (h < 0) return false;
    if (h == g_vpad_handle) return true;
    for (int i = 0; i < VPAD_MAX_SLOTS; i++) {
        if (g_slots[i].active && g_slots[i].handle == h) {
            return true;
        }
    }
    return false;
}

void vpad_poweroff_physical_ds4(void) {
    if (g_daemon_config.mode == CONFIG_MODE_SECONDARY) {
        daemon_log(LOG_LEVEL_INFO, "Skipping physical DS4 power-off (CONFIG_MODE_SECONDARY)");
        return;
    }

    vpad_ensure_subsystem();
    init_user_service();
    init_hid_control();
    vpad_close_cached_ds4_handles();

    if (g_daemon_config.mode == CONFIG_MODE_AUTO) {
        SceUserServiceLoginUserIdList chk;
        memset(&chk, 0, sizeof(chk));
        for (int i = 0; i < SCE_USER_SERVICE_MAX_LOGIN_USERS; i++) chk.userId[i] = SCE_USER_SERVICE_USER_ID_INVALID;
        if (p_sceUserServiceGetLoginUserIdList) p_sceUserServiceGetLoginUserIdList(&chk);
        int chk_logged = 0;
        for (int i = 0; i < SCE_USER_SERVICE_MAX_LOGIN_USERS; i++) {
            if (chk.userId[i] != SCE_USER_SERVICE_USER_ID_INVALID && chk.userId[i] != 0 && (uint32_t)chk.userId[i] != 0xFFFFFFFFU)
                chk_logged++;
        }
        if (chk_logged >= 2) {
            daemon_log(LOG_LEVEL_INFO, "Skipping physical DS4 power-off (AUTO mode, %d users logged in — new controller goes to Player 2)", chk_logged);
            return;
        }
        daemon_log(LOG_LEVEL_INFO, "Physical DS4 power-off: AUTO mode with %d user logged in, replacing physical DS4 on Player 1", chk_logged);
    }

    SceUserServiceLoginUserIdList list;
    memset(&list, 0, sizeof(list));
    for (int i = 0; i < SCE_USER_SERVICE_MAX_LOGIN_USERS; i++) {
        list.userId[i] = SCE_USER_SERVICE_USER_ID_INVALID;
    }

    if (p_sceUserServiceGetLoginUserIdList) {
        p_sceUserServiceGetLoginUserIdList(&list);
    }
    if (list.userId[0] == SCE_USER_SERVICE_USER_ID_INVALID && p_sceUserServiceGetInitialUser) {
        p_sceUserServiceGetInitialUser(&list.userId[0]);
    }

    bool any_disconnected = false;
    for (int i = 0; i < SCE_USER_SERVICE_MAX_LOGIN_USERS; i++) {
        SceUserServiceUserId uid = list.userId[i];
        if (uid == SCE_USER_SERVICE_USER_ID_INVALID || uid == 0 || (uint32_t)uid == 0xFFFFFFFFU) {
            continue;
        }

        int32_t pad_handle = -1;
        bool opened_temp = false;
        if (p_scePadGetHandle) {
            pad_handle = p_scePadGetHandle(uid, 0, 0);
        }
        if (pad_handle < 0 && p_scePadOpen) {
            pad_handle = p_scePadOpen(uid, 0, 0, NULL);
            if (pad_handle >= 0) {
                opened_temp = true;
            }
        }

        if (pad_handle >= 0 && !is_our_virtual_handle(pad_handle)) {
            uint8_t dummy_state[256];
            if (p_scePadReadState) {
                memset(dummy_state, 0, sizeof(dummy_state));
                p_scePadReadState(pad_handle, dummy_state);
            }
            if (p_scePadGetControllerInformation) {
                memset(dummy_state, 0, sizeof(dummy_state));
                p_scePadGetControllerInformation(pad_handle, dummy_state);
            }

            int64_t dev_id[4] = {0, 0, 0, 0};
            if (p_scePadGetDeviceId) {
                int r_id = p_scePadGetDeviceId(pad_handle, dev_id);
                if (!is_valid_dev_id(dev_id[0]) && p_scePadReadState) {
                    daemon_sys_usleep(10000);
                    memset(dummy_state, 0, sizeof(dummy_state));
                    p_scePadReadState(pad_handle, dummy_state);
                    r_id = p_scePadGetDeviceId(pad_handle, dev_id);
                }
                daemon_log(LOG_LEVEL_INFO, "Physical DS4 [user=0x%X, handle=0x%X]: scePadGetDeviceId() = 0x%08X, dev_id=0x%llX",
                           uid, pad_handle, r_id, (unsigned long long)dev_id[0]);
            }
            if (!is_valid_dev_id(dev_id[0]) && p_sceHidControlGetDeviceId) {
                dev_id[0] = 0;
                int r_hid = p_sceHidControlGetDeviceId(pad_handle, dev_id);
                daemon_log(LOG_LEVEL_INFO, "Physical DS4 [user=0x%X, handle=0x%X]: sceHidControlGetDeviceId() = 0x%08X, dev_id=0x%llX",
                           uid, pad_handle, r_hid, (unsigned long long)dev_id[0]);
            }
            if (!is_valid_dev_id(dev_id[0]) && p_sceMbusResolveByHandle) {
                dev_id[0] = 0;
                int r_mbus = p_sceMbusResolveByHandle(pad_handle, dev_id);
                daemon_log(LOG_LEVEL_INFO, "Physical DS4 [user=0x%X, handle=0x%X]: sceMbusResolveByHandle() = 0x%08X, dev_id=0x%llX",
                           uid, pad_handle, r_mbus, (unsigned long long)dev_id[0]);
            }

            if (p_scePadDisconnectPort) {
                int r = p_scePadDisconnectPort(pad_handle);
                daemon_log(LOG_LEVEL_INFO, "scePadDisconnectPort(handle=0x%X) = 0x%08X", pad_handle, r);
                if (r == 0) any_disconnected = true;
            }
            if (p_sceHidControlDisconnectPort) {
                int r = p_sceHidControlDisconnectPort(pad_handle);
                daemon_log(LOG_LEVEL_INFO, "sceHidControlDisconnectPort(handle=0x%X) = 0x%08X", pad_handle, r);
                if (r == 0) any_disconnected = true;
            }
            if (p_sceHidControlDisconnectHandle) {
                int r = p_sceHidControlDisconnectHandle(pad_handle);
                daemon_log(LOG_LEVEL_INFO, "sceHidControlDisconnectHandle(handle=0x%X) = 0x%08X", pad_handle, r);
                if (r == 0) any_disconnected = true;
            }
            if (p_scePadDisconnectDevice) {
                int r = p_scePadDisconnectDevice(pad_handle);
                daemon_log(LOG_LEVEL_INFO, "scePadDisconnectDevice(handle=0x%X) = 0x%08X", pad_handle, r);
                if (r == 0) any_disconnected = true;
            }

            if (is_valid_dev_id(dev_id[0])) {
                daemon_log(LOG_LEVEL_INFO, "Powering off physical DS4 for user 0x%X (handle=0x%X, dev_id=0x%X)...",
                           uid, pad_handle, (int32_t)dev_id[0]);
                if (p_sceMbusDisconnectDevice) {
                    int r = p_sceMbusDisconnectDevice((int32_t)dev_id[0]);
                    daemon_log(LOG_LEVEL_INFO, "sceMbusDisconnectDevice(0x%X) = 0x%08X", (int32_t)dev_id[0], r);
                }
                if (p_sceHidControlDisconnectDevice) {
                    int r = p_sceHidControlDisconnectDevice((int32_t)dev_id[0]);
                    daemon_log(LOG_LEVEL_INFO, "sceHidControlDisconnectDevice(0x%X) = 0x%08X", (int32_t)dev_id[0], r);
                }
                if (p_scePadDisconnectDevice && (int32_t)dev_id[0] != pad_handle) {
                    int r = p_scePadDisconnectDevice((int32_t)dev_id[0]);
                    daemon_log(LOG_LEVEL_INFO, "scePadDisconnectDevice(dev_id=0x%X) = 0x%08X", (int32_t)dev_id[0], r);
                }
                any_disconnected = true;
            }

            if (opened_temp && p_scePadClose) {
                p_scePadClose(pad_handle);
            }
        }
    }

    if (any_disconnected) {
        daemon_sys_usleep(80000);
    }
}

static int vpad_bind_user_slot(int slot_idx, ConfigMode mode, int *out_slot, char *out_username, size_t username_len) {
    if (slot_idx < 0 || slot_idx >= VPAD_MAX_SLOTS || !g_slots[slot_idx].active || g_slots[slot_idx].handle < 0) {
        return -1;
    }

    init_user_service();

    SceUserServiceLoginUserIdList list;
    memset(&list, 0, sizeof(list));
    for (int i = 0; i < SCE_USER_SERVICE_MAX_LOGIN_USERS; i++) {
        list.userId[i] = SCE_USER_SERVICE_USER_ID_INVALID;
    }

    if (p_sceUserServiceGetLoginUserIdList) {
        int r = p_sceUserServiceGetLoginUserIdList(&list);
        daemon_log(LOG_LEVEL_INFO, "sceUserServiceGetLoginUserIdList() = 0x%08X (u0=0x%X, u1=0x%X, u2=0x%X, u3=0x%X)",
                   r, list.userId[0], list.userId[1], list.userId[2], list.userId[3]);
    }

    SceUserServiceUserId valid_users[SCE_USER_SERVICE_MAX_LOGIN_USERS];
    int num_logged_in = 0;
    for (int i = 0; i < SCE_USER_SERVICE_MAX_LOGIN_USERS; i++) {
        if (list.userId[i] != SCE_USER_SERVICE_USER_ID_INVALID &&
            list.userId[i] != 0 &&
            (uint32_t)list.userId[i] != 0xFFFFFFFFU) {
            valid_users[num_logged_in++] = list.userId[i];
        }
    }
    s_last_login_count = num_logged_in;

    SceUserServiceUserId target_user = SCE_USER_SERVICE_USER_ID_INVALID;
    int player_slot = slot_idx + 1;

    if (mode == CONFIG_MODE_AUTO) {
        // CONFIG_MODE_AUTO:
        // Never call sceMbusBindDeviceWithUserId in AUTO mode on any slot!
        // Calling sceMbusBindDeviceWithUserId from the daemon desynchronizes SceShellCore's
        // login manager and freezes the user selection screen. Let the native PS button pulse
        // open the "Who is using this controller?" screen so SceShellCore binds the account natively.
        target_user = SCE_USER_SERVICE_USER_ID_INVALID;
        player_slot = slot_idx + 1;
        if (player_slot > 4) player_slot = 4;
    } else if (mode == CONFIG_MODE_SECONDARY || slot_idx >= 1) {
        int user_idx = (slot_idx >= 1) ? slot_idx : 1;
        if (num_logged_in > user_idx) {
            target_user = valid_users[user_idx];
            player_slot = user_idx + 1;
        } else if (num_logged_in == 1 && slot_idx >= 1 && !g_slots[0].active) {
            target_user = valid_users[0];
            player_slot = 1;
        } else {
            target_user = SCE_USER_SERVICE_USER_ID_INVALID;
            player_slot = user_idx + 1;
        }
    } else {
        if (num_logged_in >= 1) {
            target_user = valid_users[0];
        } else if (p_sceUserServiceGetInitialUser) {
            p_sceUserServiceGetInitialUser(&target_user);
        }
        player_slot = 1;
    }

    g_slots[slot_idx].assigned_slot = player_slot;
    g_slots[slot_idx].assigned_username[0] = '\0';

    if (target_user != SCE_USER_SERVICE_USER_ID_INVALID && p_sceUserServiceGetUserName) {
        p_sceUserServiceGetUserName(target_user, g_slots[slot_idx].assigned_username, sizeof(g_slots[slot_idx].assigned_username));
    } else if (mode == CONFIG_MODE_AUTO && slot_idx < num_logged_in && p_sceUserServiceGetUserName) {
        p_sceUserServiceGetUserName(valid_users[slot_idx], g_slots[slot_idx].assigned_username, sizeof(g_slots[slot_idx].assigned_username));
    }
    if (g_slots[slot_idx].assigned_username[0] == '\0') {
        snprintf(g_slots[slot_idx].assigned_username, sizeof(g_slots[slot_idx].assigned_username), "Jugador %d", player_slot);
    }

    if (slot_idx == 0) {
        s_assigned_slot = player_slot;
        strncpy(s_assigned_username, g_slots[slot_idx].assigned_username, sizeof(s_assigned_username) - 1);
        s_assigned_username[sizeof(s_assigned_username) - 1] = '\0';
    }

    if (target_user != SCE_USER_SERVICE_USER_ID_INVALID && p_sceMbusBindDeviceWithUserId) {
        int r = p_sceMbusBindDeviceWithUserId(g_slots[slot_idx].handle, target_user);
        daemon_log(LOG_LEVEL_INFO, "sceMbusBindDeviceWithUserId(slot=%d, handle=0x%X, user=0x%08X, player=%d, name='%s') = 0x%08X",
                   slot_idx, g_slots[slot_idx].handle, target_user, player_slot, g_slots[slot_idx].assigned_username, r);
        if (r < 0) {
            daemon_log(LOG_LEVEL_WARN, "sceMbusBindDeviceWithUserId failed (0x%08X) — falling back to PS button pulse", r);
            vpad_pulse_ps_button_slot(slot_idx);
        }
    } else {
        daemon_log(LOG_LEVEL_INFO, "Virtual DS4 slot %d awaiting user selection (mode=%d, player=%d) — pulsing PS button",
                   slot_idx, mode, player_slot);
        vpad_pulse_ps_button_slot(slot_idx);
    }

    if (out_slot) *out_slot = player_slot;
    if (out_username && username_len > 0) {
        strncpy(out_username, g_slots[slot_idx].assigned_username, username_len - 1);
        out_username[username_len - 1] = '\0';
    }

    return 0;
}

int vpad_bind_user(ConfigMode mode, int *out_slot, char *out_username, size_t username_len) {
    for (int i = 0; i < VPAD_MAX_SLOTS; i++) {
        if (g_slots[i].active) {
            return vpad_bind_user_slot(i, mode, out_slot, out_username, username_len);
        }
    }
    return -1;
}

static void vpad_schedule_ps_pulse_slot(int slot, uint64_t delay_us, uint64_t duration_us) {
    if (slot < 0 || slot >= VPAD_MAX_SLOTS || !g_slots[slot].active || g_slots[slot].handle < 0 || !p_scePadVirtualDeviceInsertData) {
        return;
    }

    uint64_t now = daemon_sys_time_us();
    g_slots[slot].ps_pulse_start_us = now + delay_us;
    g_slots[slot].ps_pulse_until_us = now + delay_us + duration_us;
    g_slots[slot].ps_pulse_active = true;

    if (delay_us == 0) {
        daemon_log(LOG_LEVEL_INFO, "vpad_pulse_ps_button_slot(%d): Holding PS button for %llums to trigger native user picker...",
                   slot, (unsigned long long)(duration_us / 1000ULL));

        OrbisPadData pulse;
        memset(&pulse, 0, sizeof(pulse));
        pulse.leftStick.x = 128;
        pulse.leftStick.y = 128;
        pulse.rightStick.x = 128;
        pulse.rightStick.y = 128;
        pulse.connected = 1;
        pulse.count = 1;
        pulse.buttons = DS4_BUTTON_PS;
        pulse.timestamp = now;
        pulse.quat.w = 1.0f;
        pulse.acell.y = -1.0f;

        p_scePadVirtualDeviceInsertData(g_slots[slot].handle, &pulse);
        g_slots[slot].last_send_time_us = now;
        g_slots[slot].last_sent_data = pulse;
        g_slots[slot].has_sent_first = true;
    } else {
        daemon_log(LOG_LEVEL_INFO, "vpad_schedule_ps_pulse_slot(%d): Scheduled PS pulse in %llums (%llums hold)",
                   slot, (unsigned long long)(delay_us / 1000ULL), (unsigned long long)(duration_us / 1000ULL));
    }
}

void vpad_pulse_ps_button_slot(int slot) {
    vpad_schedule_ps_pulse_slot(slot, 0, 140000ULL);
}

void vpad_reopen_user_picker_slot(int slot) {
    if (slot < 0 || slot >= VPAD_MAX_SLOTS || !g_slots[slot].active || g_slots[slot].handle < 0) {
        return;
    }
    daemon_log(LOG_LEVEL_INFO, "vpad_reopen_user_picker_slot(%d): Pulsing PS button on handle=0x%X to open native PS4 user picker",
               slot, g_slots[slot].handle);
    g_slots[slot].ps_followup_pending = false;
    vpad_schedule_ps_pulse_slot(slot, 0, 160000ULL);
}

void vpad_pulse_ps_button(void) {
    for (int i = VPAD_MAX_SLOTS - 1; i >= 0; i--) {
        if (g_slots[i].active) {
            vpad_pulse_ps_button_slot(i);
            return;
        }
    }
}

void vpad_reactivate_slot(int slot) {
    if (slot < 0 || slot >= VPAD_MAX_SLOTS || !g_slots[slot].active || g_slots[slot].handle < 0) {
        return;
    }
    if (p_scePadSetConnection) {
        int r = p_scePadSetConnection(g_slots[slot].handle, 1);
        daemon_log(LOG_LEVEL_INFO, "vpad_reactivate_slot(%d): scePadSetConnection(0x%X, 1) = 0x%08X",
                   slot, g_slots[slot].handle, r);
    }
    if (p_scePadVirtualDeviceInsertData) {
        OrbisPadData neutral;
        memset(&neutral, 0, sizeof(neutral));
        neutral.leftStick.x = 128;
        neutral.leftStick.y = 128;
        neutral.rightStick.x = 128;
        neutral.rightStick.y = 128;
        neutral.connected = 1;
        neutral.count = 1;
        neutral.timestamp = daemon_sys_time_us();
        neutral.quat.w = 1.0f;
        neutral.acell.y = -1.0f;
        p_scePadVirtualDeviceInsertData(g_slots[slot].handle, &neutral);
        g_slots[slot].last_send_time_us = neutral.timestamp;
        g_slots[slot].last_sent_data = neutral;
        g_slots[slot].has_sent_first = true;
    }
    g_slots[slot].user_interacted = false;
    g_slots[slot].slot_open_login_count = (s_last_login_count >= 0) ? s_last_login_count : 0;
    vpad_schedule_ps_pulse_slot(slot, 150000ULL, 160000ULL);
    if (g_daemon_config.mode != CONFIG_MODE_AUTO) {
        vpad_bind_user_slot(slot, g_daemon_config.mode, NULL, NULL, 0);
    }
}

static void compute_rainbow_rgb(uint64_t now_us, uint8_t *out_r, uint8_t *out_g, uint8_t *out_b) {
    // Full 360-degree rainbow cycle every 6 seconds (6,000,000 us)
    uint32_t phase = (uint32_t)((now_us / 3921ULL) % 1530ULL); // 0..1529 (6 * 255)
    uint32_t region = phase / 255;
    uint8_t rem = (uint8_t)(phase % 255);
    switch (region) {
        case 0: *out_r = 255;       *out_g = rem;       *out_b = 0;         break;
        case 1: *out_r = 255 - rem; *out_g = 255;       *out_b = 0;         break;
        case 2: *out_r = 0;         *out_g = 255;       *out_b = rem;       break;
        case 3: *out_r = 0;         *out_g = 255 - rem; *out_b = 255;       break;
        case 4: *out_r = rem;       *out_g = 0;         *out_b = 255;       break;
        default:*out_r = 255;       *out_g = 0;         *out_b = 255 - rem; break;
    }
}

static uint8_t compute_breathing_scale(uint64_t now_us) {
    // Smooth triangle wave breathing cycle every 3.0 seconds (10% .. 100%)
    uint32_t ms = (uint32_t)((now_us / 1000ULL) % 3000ULL);
    uint32_t pct = (ms < 1500) ? (10 + (ms * 90) / 1500) : (100 - ((ms - 1500) * 90) / 1500);
    if (pct < 8) pct = 8;
    if (pct > 100) pct = 100;
    return (uint8_t)pct;
}

void vpad_apply_ds4_lightbar(void) {
    if (g_vpad_active) {
        vpad_close_cached_ds4_handles();
        return;
    }
    if (vpad_ensure_subsystem() != 0) {
        return;
    }
    init_user_service();

    static const uint8_t ds4_colors[7][3] = {
        { 0, 112, 220 },   // 0: Azul PS4
        { 239, 68, 68 },   // 1: Rojo Carmesi
        { 16, 185, 129 },  // 2: Verde Esmeralda
        { 168, 85, 247 },  // 3: Purpura Neon
        { 6, 182, 212 },   // 4: Cian Electrico
        { 245, 158, 11 },  // 5: Ambar Oro
        { 248, 250, 252 }  // 6: Blanco Puro
    };

    SceUserServiceLoginUserIdList list;
    memset(&list, 0, sizeof(list));
    for (int i = 0; i < SCE_USER_SERVICE_MAX_LOGIN_USERS; i++) {
        list.userId[i] = SCE_USER_SERVICE_USER_ID_INVALID;
    }
    if (p_sceUserServiceGetLoginUserIdList) {
        p_sceUserServiceGetLoginUserIdList(&list);
    }
    if (list.userId[0] == SCE_USER_SERVICE_USER_ID_INVALID && p_sceUserServiceGetInitialUser) {
        p_sceUserServiceGetInitialUser(&list.userId[0]);
    }

    uint64_t now_us = daemon_sys_time_us();

    for (int i = 0; i < SCE_USER_SERVICE_MAX_LOGIN_USERS; i++) {
        SceUserServiceUserId uid = list.userId[i];
        if (uid == SCE_USER_SERVICE_USER_ID_INVALID || uid == 0 || (uint32_t)uid == 0xFFFFFFFFU) {
            if (s_ds4_handles[i] >= 0) {
                if (p_scePadClose) p_scePadClose(s_ds4_handles[i]);
                s_ds4_handles[i] = -1;
                s_ds4_uids[i] = SCE_USER_SERVICE_USER_ID_INVALID;
            }
            continue;
        }

        // Reuse persistent handle so libScePad never resets lightbar on scePadClose!
        int32_t pad_handle = -1;
        if (p_scePadGetHandle) {
            pad_handle = p_scePadGetHandle(uid, 0, 0);
        }
        if (pad_handle < 0) {
            if (s_ds4_handles[i] >= 0 && s_ds4_uids[i] == uid) {
                OrbisPadData test_state;
                memset(&test_state, 0, sizeof(test_state));
                if (p_scePadReadState && p_scePadReadState(s_ds4_handles[i], &test_state) == 0 && test_state.connected) {
                    pad_handle = s_ds4_handles[i];
                } else {
                    if (p_scePadClose) p_scePadClose(s_ds4_handles[i]);
                    s_ds4_handles[i] = -1;
                }
            }
            if (pad_handle < 0 && p_scePadOpen) {
                pad_handle = p_scePadOpen(uid, 0, 0, NULL);
                if (pad_handle >= 0) {
                    s_ds4_handles[i] = pad_handle;
                    s_ds4_uids[i] = uid;
                }
            }
        }

        if (pad_handle >= 0 && !is_our_virtual_handle(pad_handle)) {
            int br = g_daemon_config.ds4_brightness;
            if (br < 0 || br > 2) br = 0;

            int hw_base_br = (g_daemon_config.ds4_lightbar_mode == 3) ? 2 : br;
            if (p_sceUserServiceSetLightBarBaseBrightness) {
                p_sceUserServiceSetLightBarBaseBrightness(uid, hw_base_br);
            }
            if (p_scePadSetLightBarBaseBrightness) {
                p_scePadSetLightBarBaseBrightness(pad_handle, hw_base_br);
            }

            if (g_daemon_config.ds4_lightbar_mode == 0 && br == 0) {
                if (p_scePadResetLightBar) {
                    p_scePadResetLightBar(pad_handle);
                }
            } else if (p_scePadSetLightBar) {
                OrbisPadColor col = { 0, 0, 0, 0 };
                int idx = g_daemon_config.ds4_lightbar_color;
                if (idx < 0 || idx > 6) idx = 0;

                if (g_daemon_config.ds4_lightbar_mode == 3) {
                    col.r = 0; col.g = 0; col.b = 0; col.a = 0;
                    int r_off = p_scePadSetLightBar(pad_handle, &col);
                    if (r_off != 0) {
                        col.r = 1; col.g = 1; col.b = 0; col.a = 0;
                        p_scePadSetLightBar(pad_handle, &col);
                    }
                    continue;
                } else if (g_daemon_config.ds4_lightbar_mode == 2) {
                    col.r = 16; col.g = 220; col.b = 110;
                } else if (g_daemon_config.ds4_lightbar_mode == 4) {
                    compute_rainbow_rgb(now_us, &col.r, &col.g, &col.b);
                } else if (g_daemon_config.ds4_lightbar_mode == 5) {
                    uint8_t bscale = compute_breathing_scale(now_us);
                    col.r = (uint8_t)((ds4_colors[idx][0] * bscale) / 100);
                    col.g = (uint8_t)((ds4_colors[idx][1] * bscale) / 100);
                    col.b = (uint8_t)((ds4_colors[idx][2] * bscale) / 100);
                } else if (g_daemon_config.ds4_lightbar_mode == 0) {
                    col.r = ds4_colors[0][0];
                    col.g = ds4_colors[0][1];
                    col.b = ds4_colors[0][2];
                } else {
                    col.r = ds4_colors[idx][0];
                    col.g = ds4_colors[idx][1];
                    col.b = ds4_colors[idx][2];
                }

                if (br == 1) {
                    uint8_t orig_r = col.r, orig_g = col.g, orig_b = col.b;
                    col.r = (uint8_t)((col.r * 25) / 100);
                    col.g = (uint8_t)((col.g * 25) / 100);
                    col.b = (uint8_t)((col.b * 25) / 100);
                    if (orig_r > 0 && col.r == 0) col.r = 1;
                    if (orig_g > 0 && col.g == 0) col.g = 1;
                    if (orig_b > 0 && col.b == 0) col.b = 1;
                } else if (br == 2) {
                    uint8_t orig_r = col.r, orig_g = col.g, orig_b = col.b;
                    col.r = (uint8_t)((col.r * 5) / 100);
                    col.g = (uint8_t)((col.g * 5) / 100);
                    col.b = (uint8_t)((col.b * 5) / 100);
                    if (orig_r > 0 && col.r == 0) col.r = 2;
                    if (orig_g > 0 && col.g == 0) col.g = 2;
                    if (orig_b > 0 && col.b == 0) col.b = 2;
                }
                p_scePadSetLightBar(pad_handle, &col);
            }
        }
    }
}

int vpad_get_assigned_slot(void) {
    return s_assigned_slot;
}

const char* vpad_get_assigned_user(void) {
    return s_assigned_username;
}

const char* vpad_get_assigned_user_slot(int slot) {
    if (slot >= 0 && slot < VPAD_MAX_SLOTS && g_slots[slot].active && g_slots[slot].assigned_username[0] != '\0') {
        return g_slots[slot].assigned_username;
    }
    return "Jugador";
}

bool vpad_check_user_rebind(ConfigMode mode) {
    if (!g_vpad_active || !p_sceUserServiceGetLoginUserIdList) {
        return false;
    }

    SceUserServiceLoginUserIdList list;
    memset(&list, 0, sizeof(list));
    for (int i = 0; i < SCE_USER_SERVICE_MAX_LOGIN_USERS; i++) {
        list.userId[i] = SCE_USER_SERVICE_USER_ID_INVALID;
    }
    if (p_sceUserServiceGetLoginUserIdList(&list) != 0) {
        return false;
    }

    SceUserServiceUserId valid_users[SCE_USER_SERVICE_MAX_LOGIN_USERS];
    int num_logged_in = 0;
    for (int i = 0; i < SCE_USER_SERVICE_MAX_LOGIN_USERS; i++) {
        if (list.userId[i] != SCE_USER_SERVICE_USER_ID_INVALID &&
            list.userId[i] != 0 &&
            (uint32_t)list.userId[i] != 0xFFFFFFFFU) {
            valid_users[num_logged_in++] = list.userId[i];
        }
    }

    if (num_logged_in != s_last_login_count && s_last_login_count != -1) {
        int prev_count = s_last_login_count;
        daemon_log(LOG_LEVEL_INFO, "User login count changed (%d -> %d)",
                   prev_count, num_logged_in);
        s_last_login_count = num_logged_in;
        for (int i = 0; i < VPAD_MAX_SLOTS; i++) {
            if (g_slots[i].active && g_slots[i].handle >= 0) {
                if (num_logged_in > g_slots[i].slot_open_login_count) {
                    g_slots[i].ps_followup_pending = false;
                }
                if (p_scePadSetConnection) {
                    p_scePadSetConnection(g_slots[i].handle, 1);
                }
                if (mode != CONFIG_MODE_AUTO) {
                    vpad_bind_user_slot(i, mode, NULL, NULL, 0);
                } else {
                    if (i < num_logged_in && p_sceUserServiceGetUserName) {
                        g_slots[i].assigned_username[0] = '\0';
                        p_sceUserServiceGetUserName(valid_users[i], g_slots[i].assigned_username, sizeof(g_slots[i].assigned_username));
                        if (g_slots[i].assigned_username[0] == '\0') {
                            snprintf(g_slots[i].assigned_username, sizeof(g_slots[i].assigned_username), "Jugador %d", i + 1);
                        }
                        if (i == 0) {
                            strncpy(s_assigned_username, g_slots[0].assigned_username, sizeof(s_assigned_username) - 1);
                            s_assigned_username[sizeof(s_assigned_username) - 1] = '\0';
                        }
                    }
                    if (num_logged_in == 0) {
                        vpad_schedule_ps_pulse_slot(i, 150000ULL, 160000ULL);
                    }
                }
            }
        }
        return true;
    } else if (s_last_login_count == -1) {
        s_last_login_count = num_logged_in;
    }
    return false;
}

static int vpad_ensure_subsystem(void) {
    // Elevate credentials to SceShellCore system service (authid 0x3800000000010003ULL)
    // so SceShellCore login manager trusts virtual devices and routes user picker input
    vpad_elevate_creds(0);

    // 1. Mandatory: libSceMbus must be loaded prior to libScePad
    if (g_hMbus == 0) {
        if (load_prx("libSceMbus.sprx", &g_hMbus) < 0) {
            daemon_log(LOG_LEVEL_WARN, "Failed to load libSceMbus.sprx");
        } else {
            daemon_log(LOG_LEVEL_INFO, "libSceMbus.sprx loaded successfully (handle=0x%X)", g_hMbus);
            daemon_sys_dlsym(g_hMbus, "sceMbusInit", (void**)&p_sceMbusInit);
            daemon_sys_dlsym(g_hMbus, "sceMbusBindDeviceWithUserId", (void**)&p_sceMbusBindDeviceWithUserId);
            daemon_sys_dlsym(g_hMbus, "sceMbusDisconnectDevice", (void**)&p_sceMbusDisconnectDevice);
            daemon_sys_dlsym(g_hMbus, "sceMbusResolveByHandle", (void**)&p_sceMbusResolveByHandle);
            daemon_sys_dlsym(g_hMbus, "sceMbusUserLogin", (void**)&p_sceMbusUserLogin);
            daemon_sys_dlsym(g_hMbus, "sceMbusUserLogout", (void**)&p_sceMbusUserLogout);
            if (p_sceMbusInit) {
                int r = p_sceMbusInit();
                daemon_log(LOG_LEVEL_INFO, "sceMbusInit() = 0x%08X", r);
            }
        }
    }

    // 2. Load libScePad and initialize Pad/Mbus once
    if (g_hPad == 0) {
        if (load_prx("libScePad.sprx", &g_hPad) < 0) {
            daemon_log(LOG_LEVEL_ERROR, "Failed to load libScePad.sprx");
            return -1;
        }
        daemon_log(LOG_LEVEL_INFO, "libScePad.sprx loaded successfully (handle=0x%X)", g_hPad);

        // 3. Resolve Pad Symbols
        daemon_sys_dlsym(g_hPad, "scePadInit", (void**)&p_scePadInit);
        daemon_sys_dlsym(g_hPad, "scePadMbusInit", (void**)&p_scePadMbusInit);
        daemon_sys_dlsym(g_hPad, "scePadSetProcessPrivilege", (void**)&p_scePadSetProcessPrivilege);
        daemon_sys_dlsym(g_hPad, "scePadSetConnection", (void**)&p_scePadSetConnection);
        daemon_sys_dlsym(g_hPad, "scePadVirtualDeviceAddDevice", (void**)&p_scePadVirtualDeviceAddDevice);
        daemon_sys_dlsym(g_hPad, "scePadVirtualDeviceInsertData", (void**)&p_scePadVirtualDeviceInsertData);
        daemon_sys_dlsym(g_hPad, "scePadVirtualDeviceDeleteDevice", (void**)&p_scePadVirtualDeviceDeleteDevice);
        daemon_sys_dlsym(g_hPad, "scePadVirtualDeviceGetRemoteSetting", (void**)&p_scePadVirtualDeviceGetRemoteSetting);
        daemon_sys_dlsym(g_hPad, "scePadGetHandle", (void**)&p_scePadGetHandle);
        daemon_sys_dlsym(g_hPad, "scePadOpen", (void**)&p_scePadOpen);
        daemon_sys_dlsym(g_hPad, "scePadClose", (void**)&p_scePadClose);
        daemon_sys_dlsym(g_hPad, "scePadReadState", (void**)&p_scePadReadState);
        daemon_sys_dlsym(g_hPad, "scePadGetControllerInformation", (void**)&p_scePadGetControllerInformation);
        daemon_sys_dlsym(g_hPad, "scePadGetDeviceId", (void**)&p_scePadGetDeviceId);
        daemon_sys_dlsym(g_hPad, "scePadDisconnectPort", (void**)&p_scePadDisconnectPort);
        daemon_sys_dlsym(g_hPad, "scePadDisconnectDevice", (void**)&p_scePadDisconnectDevice);
        daemon_sys_dlsym(g_hPad, "scePadSetLightBar", (void**)&p_scePadSetLightBar);
        daemon_sys_dlsym(g_hPad, "scePadResetLightBar", (void**)&p_scePadResetLightBar);
        daemon_sys_dlsym(g_hPad, "scePadSetLightBarBaseBrightness", (void**)&p_scePadSetLightBarBaseBrightness);

        // Fallback: check if sceMbusBindDeviceWithUserId is exported from libScePad
        if (!p_sceMbusBindDeviceWithUserId) {
            daemon_sys_dlsym(g_hPad, "sceMbusBindDeviceWithUserId", (void**)&p_sceMbusBindDeviceWithUserId);
        }

        if (p_scePadInit) {
            int r = p_scePadInit();
            daemon_log(LOG_LEVEL_INFO, "scePadInit() = 0x%08X", r);
            if (r != 0) {
                daemon_log(LOG_LEVEL_WARN, "scePadInit failed (0x%08X), retrying with original credentials...", r);
                vpad_elevate_creds(1);
                r = p_scePadInit();
                daemon_log(LOG_LEVEL_INFO, "scePadInit() with orig creds = 0x%08X", r);
                vpad_elevate_creds(0);
            }
        }
        if (p_scePadMbusInit) {
            int r = p_scePadMbusInit();
            daemon_log(LOG_LEVEL_INFO, "scePadMbusInit() = 0x%08X", r);
            if (r != 0) {
                vpad_elevate_creds(1);
                int r2 = p_scePadMbusInit();
                daemon_log(LOG_LEVEL_INFO, "scePadMbusInit() with orig creds = 0x%08X", r2);
                vpad_elevate_creds(0);
            }
        }
        if (p_scePadSetProcessPrivilege) {
            int r = p_scePadSetProcessPrivilege(1);
            daemon_log(LOG_LEVEL_INFO, "scePadSetProcessPrivilege(1) = 0x%08X", r);
            if (r != 0) {
                vpad_elevate_creds(1);
                int r2 = p_scePadSetProcessPrivilege(1);
                daemon_log(LOG_LEVEL_INFO, "scePadSetProcessPrivilege(1) with orig creds = 0x%08X", r2);
                vpad_elevate_creds(0);
            }
        }
    }

    if (!p_scePadVirtualDeviceAddDevice || !p_scePadVirtualDeviceInsertData) {
        daemon_log(LOG_LEVEL_ERROR, "VirtualDevice APIs not exported by libScePad");
        return -2;
    }

    return 0;
}

int vpad_slot_open_dev(VPadSource source, uint64_t device_key, int *out_slot) {
    if (vpad_ensure_subsystem() != 0) {
        return -1;
    }

    vpad_close_cached_ds4_handles();

    // Check if this physical device already holds a slot
    for (int i = 0; i < VPAD_MAX_SLOTS; i++) {
        if (g_slots[i].active && g_slots[i].source == source && g_slots[i].device_key == device_key) {
            if (out_slot) *out_slot = i;
            return 0;
        }
    }

    // Find first free slot
    int slot = -1;
    for (int i = 0; i < VPAD_MAX_SLOTS; i++) {
        if (!g_slots[i].active) {
            slot = i;
            break;
        }
    }

    if (slot < 0) {
        daemon_log(LOG_LEVEL_WARN, "vpad_slot_open: All %d virtual pad slots are occupied", VPAD_MAX_SLOTS);
        return -1;
    }

    // Only power off physical DS4 in explicit MERGE mode for USB controllers.
    // In AUTO mode (and for all Bluetooth DualSense connections), the native PS4
    // "Who is using this controller?" screen handles profile selection and automatically
    // replaces Player 1's controller or assigns Player 2/3/4 without freezing SceShellCore!
    bool any_prior_active = false;
    for (int i = 0; i < VPAD_MAX_SLOTS; i++) {
        if (g_slots[i].active) { any_prior_active = true; break; }
    }
    if (!any_prior_active && g_daemon_config.mode == CONFIG_MODE_MERGE && source == VPAD_SOURCE_USB) {
        vpad_poweroff_physical_ds4();
    }

    ScePadVirtualDeviceParam param;
    memset(&param, 0, sizeof(param));
    param.size = sizeof(param);
    param.type = 1; // Device Type 1 = DualShock 4
    param.flags = 0;

    int vHandle = -1;
    int ret = p_scePadVirtualDeviceAddDevice(&param, &vHandle);
    if (ret < 0 || vHandle < 0) {
        daemon_log(LOG_LEVEL_ERROR, "scePadVirtualDeviceAddDevice(type 1) failed for slot %d: 0x%08X", slot, ret);
        return ret;
    }

    memset(&g_slots[slot], 0, sizeof(VPadSlotState));
    g_slots[slot].handle = vHandle;
    g_slots[slot].active = true;
    g_slots[slot].source = source;
    g_slots[slot].device_key = device_key;
    g_slots[slot].assigned_slot = slot + 1;
    snprintf(g_slots[slot].assigned_username, sizeof(g_slots[slot].assigned_username), "Jugador %d", slot + 1);

    g_vpad_active = true;
    g_vpad_handle = vHandle;

    daemon_log(LOG_LEVEL_INFO, "Virtual DS4 Slot %d (%s, dev_key=0x%llX) registered with handle=0x%X",
               slot, source == VPAD_SOURCE_BT ? "Bluetooth" : "USB", (unsigned long long)device_key, vHandle);

    init_user_service();
    int cur_logins = 0;
    if (p_sceUserServiceGetLoginUserIdList) {
        SceUserServiceLoginUserIdList ulist;
        memset(&ulist, 0, sizeof(ulist));
        for (int i = 0; i < SCE_USER_SERVICE_MAX_LOGIN_USERS; i++) ulist.userId[i] = SCE_USER_SERVICE_USER_ID_INVALID;
        if (p_sceUserServiceGetLoginUserIdList(&ulist) == 0) {
            for (int i = 0; i < SCE_USER_SERVICE_MAX_LOGIN_USERS; i++) {
                if (ulist.userId[i] != SCE_USER_SERVICE_USER_ID_INVALID && ulist.userId[i] != 0 && (uint32_t)ulist.userId[i] != 0xFFFFFFFFU) {
                    cur_logins++;
                }
            }
            s_last_login_count = cur_logins;
        }
    }
    g_slots[slot].slot_open_login_count = cur_logins;

    if (p_scePadSetConnection) {
        int r = p_scePadSetConnection(vHandle, 1);
        daemon_log(LOG_LEVEL_INFO, "scePadSetConnection(0x%X, 1) = 0x%08X", vHandle, r);
    }

    // Send initial neutral packet so Mbus/SceShellCore enumerates an active idle DS4 BEFORE seeing the PS press edge
    if (p_scePadVirtualDeviceInsertData) {
        OrbisPadData neutral;
        memset(&neutral, 0, sizeof(neutral));
        neutral.leftStick.x = 128;
        neutral.leftStick.y = 128;
        neutral.rightStick.x = 128;
        neutral.rightStick.y = 128;
        neutral.connected = 1;
        neutral.count = 1;
        neutral.timestamp = daemon_sys_time_us();
        neutral.quat.w = 1.0f;
        neutral.acell.y = -1.0f;
        int ri = p_scePadVirtualDeviceInsertData(vHandle, &neutral);
        g_slots[slot].last_send_time_us = neutral.timestamp;
        g_slots[slot].last_sent_data = neutral;
        g_slots[slot].has_sent_first = true;
        daemon_log(LOG_LEVEL_INFO, "Virtual DS4 Slot %d initial neutral packet -> 0x%08X", slot, ri);
    }

    // Trigger native PS4 "Who is using this controller?" user picker via delayed 160ms PS pulse (after 150ms Mbus settle)
    vpad_schedule_ps_pulse_slot(slot, 150000ULL, 160000ULL);
    // For 2nd+ controllers (slot >= 1), schedule one gentle fallback pulse at +1.8s ONLY if no input/login happened
    if (slot >= 1) {
        g_slots[slot].ps_followup_pending = true;
        g_slots[slot].ps_followup_at_us = daemon_sys_time_us() + 1800000ULL;
    }

    if (g_daemon_config.mode != CONFIG_MODE_AUTO) {
        vpad_bind_user_slot(slot, g_daemon_config.mode, NULL, NULL, 0);
    }

    if (out_slot) *out_slot = slot;
    return 0;
}

int vpad_slot_open(VPadSource source, int *out_slot) {
    return vpad_slot_open_dev(source, 0, out_slot);
}

void vpad_slot_close_dev(VPadSource source, uint64_t device_key) {
    for (int i = 0; i < VPAD_MAX_SLOTS; i++) {
        if (g_slots[i].active && g_slots[i].source == source && g_slots[i].device_key == device_key) {
            vpad_slot_close(i);
            return;
        }
    }
}

void vpad_slot_close(int slot) {
    if (slot < 0 || slot >= VPAD_MAX_SLOTS || !g_slots[slot].active) {
        return;
    }

    int handle = g_slots[slot].handle;
    daemon_log(LOG_LEVEL_INFO, "Closing Virtual Pad Slot %d (handle=0x%X, source=%d)",
               slot, handle, g_slots[slot].source);

    if (handle >= 0) {
        if (p_scePadSetConnection) {
            p_scePadSetConnection(handle, 0);
        }
        if (p_sceMbusDisconnectDevice) {
            p_sceMbusDisconnectDevice(handle);
        }
        daemon_sys_usleep(30000);
        if (p_scePadVirtualDeviceDeleteDevice) {
            p_scePadVirtualDeviceDeleteDevice(handle);
        }
    }

    memset(&g_slots[slot], 0, sizeof(VPadSlotState));
    g_slots[slot].handle = -1;

    bool any_active = false;
    for (int i = 0; i < VPAD_MAX_SLOTS; i++) {
        if (g_slots[i].active) {
            any_active = true;
            g_vpad_handle = g_slots[i].handle;
            break;
        }
    }
    if (!any_active) {
        g_vpad_active = false;
        g_vpad_handle = -1;
    }
}

bool vpad_slot_is_active(int slot) {
    if (slot >= 0 && slot < VPAD_MAX_SLOTS) {
        return g_slots[slot].active;
    }
    return false;
}

int vpad_get_assigned_slot_num(int slot) {
    if (slot >= 0 && slot < VPAD_MAX_SLOTS && g_slots[slot].active) {
        return g_slots[slot].assigned_slot;
    }
    return 1;
}

int vpad_init(void) {
    int slot = -1;
    return vpad_slot_open(VPAD_SOURCE_USB, &slot);
}

void vpad_shutdown(void) {
    vpad_close_cached_ds4_handles();
    for (int i = 0; i < VPAD_MAX_SLOTS; i++) {
        if (g_slots[i].active) {
            vpad_slot_close(i);
        }
    }
    g_vpad_active = false;
    g_vpad_handle = -1;
    vpad_elevate_creds(1);
}

bool vpad_is_active(void) {
    return g_vpad_active;
}

int vpad_send_data_slot(int slot, const OrbisPadData *data) {
    if (slot < 0 || slot >= VPAD_MAX_SLOTS || !g_slots[slot].active || g_slots[slot].handle < 0 || !p_scePadVirtualDeviceInsertData || !data) {
        return -1;
    }

    VPadSlotState *s = &g_slots[slot];
    uint64_t now = daemon_sys_time_us();

    // Check for active touch or motion inputs (angular velocity in ScePadData offset 0x2C / OpenOrbis 'acell')
    bool has_touch = (data->touch.fingers > 0) || ((data->buttons & DS4_BUTTON_TOUCHPAD) != 0);
    bool has_motion = (data->acell.x > 0.08f || data->acell.x < -0.08f ||
                       data->acell.y > 0.08f || data->acell.y < -0.08f ||
                       data->acell.z > 0.08f || data->acell.z < -0.08f);

    // Determine if pad is in neutral / resting state
    bool is_neutral = (data->buttons == 0 &&
                       !has_touch && !has_motion &&
                       data->analogButtons.l2 <= 4 &&
                       data->analogButtons.r2 <= 4 &&
                       abs_diff(data->leftStick.x, 128) <= 4 &&
                       abs_diff(data->leftStick.y, 128) <= 4 &&
                       abs_diff(data->rightStick.x, 128) <= 4 &&
                       abs_diff(data->rightStick.y, 128) <= 4);

    bool pulse_window_active = (s->ps_pulse_active && now >= s->ps_pulse_start_us);

    bool state_changed = false;
    if (!s->has_sent_first || pulse_window_active) {
        state_changed = true;
    } else if (data->buttons != s->last_sent_data.buttons) {
        state_changed = true;
    } else if (abs_diff(data->leftStick.x, s->last_sent_data.leftStick.x) > 1 ||
               abs_diff(data->leftStick.y, s->last_sent_data.leftStick.y) > 1 ||
               abs_diff(data->rightStick.x, s->last_sent_data.rightStick.x) > 1 ||
               abs_diff(data->rightStick.y, s->last_sent_data.rightStick.y) > 1) {
        state_changed = true;
    } else if (abs_diff(data->analogButtons.l2, s->last_sent_data.analogButtons.l2) > 2 ||
               abs_diff(data->analogButtons.r2, s->last_sent_data.analogButtons.r2) > 2) {
        state_changed = true;
    } else if (data->touch.fingers != s->last_sent_data.touch.fingers ||
               (data->touch.fingers > 0 && (data->touch.touch[0].x != s->last_sent_data.touch.touch[0].x ||
                                            data->touch.touch[0].y != s->last_sent_data.touch.touch[0].y))) {
        state_changed = true;
    } else if (has_motion) {
        state_changed = true;
    }

    // Rate-limiting: only throttle when pad is completely neutral/untouched (keep-alive at 60 Hz = ~16.6ms)
    if (is_neutral && !state_changed && !pulse_window_active) {
        if ((now - s->last_send_time_us) < 16666ULL) {
            return 0; // Throttled cleanly when idle
        }
    }

    // Sequential rolling counter (1..255, wrapping to 1) adhering to DualShock 4 hardware spec
    if (++s->packet_counter == 0) {
        s->packet_counter = 1;
    }

    // Cancel pending follow-up PS pulse on any user input so we never dismiss an already-open picker
    if ((data->buttons != 0) ||
        abs_diff(data->leftStick.x, 128) > 32 ||
        abs_diff(data->leftStick.y, 128) > 32) {
        s->user_interacted = true;
        if (s->ps_followup_pending) {
            s->ps_followup_pending = false;
        }
    }

    OrbisPadData send_data = *data;
    if (s->ps_pulse_active) {
        if (now >= s->ps_pulse_until_us) {
            s->ps_pulse_active = false;
            daemon_log(LOG_LEVEL_INFO, "vpad_pulse_ps_button_slot(%d): PS button pulse complete", slot);
        } else if (now >= s->ps_pulse_start_us) {
            send_data.buttons |= DS4_BUTTON_PS;
        }
    }
    // ScePadData offset 104 is connectedCount (must be 1 for valid connected state)
    send_data.count = 1;
    if (send_data.timestamp == 0) {
        send_data.timestamp = now;
    }

    // Identity orientation quaternion and downward resting gravity
    if (send_data.quat.x == 0.0f && send_data.quat.y == 0.0f &&
        send_data.quat.z == 0.0f && send_data.quat.w == 0.0f) {
        send_data.quat.w = 1.0f;
    }
    if (send_data.acell.x == 0.0f && send_data.acell.y == 0.0f && send_data.acell.z == 0.0f) {
        send_data.acell.y = -1.0f;
    }

    // Capacitive Touchpad Emulation for Touchpad Click:
    if ((send_data.buttons & DS4_BUTTON_TOUCHPAD) && send_data.touch.fingers == 0) {
        send_data.touch.fingers = 1;
        send_data.touch.touch[0].finger = 0;
        send_data.touch.touch[0].x = 960;
        send_data.touch.touch[0].y = 470;
    }

    int ret = p_scePadVirtualDeviceInsertData(s->handle, &send_data);
    s->last_send_time_us = now;
    s->last_sent_data = send_data;
    s->has_sent_first = true;

    int ps_now = (send_data.buttons & DS4_BUTTON_PS) ? 1 : 0;
    if (ps_now != s->last_ps_state) {
        s->last_ps_state = ps_now;
        daemon_log(LOG_LEVEL_INFO, "vpad_send_data_slot(%d): PS Button %s (btn=0x%08X, ret=0x%08X)",
                   slot, ps_now ? "ACTIVE" : "RELEASED", send_data.buttons, ret);
    }

    if (s->logged_insert_packets < 5 || (ret != 0 && s->logged_insert_packets < 25) ||
        (data->buttons != s->last_logged_buttons && s->logged_insert_packets < 60)) {
        daemon_log(LOG_LEVEL_INFO, "InsertData(slot=%d, h=0x%X, btn=0x%08X, lx=%d, ly=%d) -> 0x%08X",
                   slot, s->handle, send_data.buttons, send_data.leftStick.x, send_data.leftStick.y, ret);
        s->last_logged_buttons = data->buttons;
        s->logged_insert_packets++;
    }

    return ret;
}

int vpad_send_data(const OrbisPadData *data) {
    for (int i = 0; i < VPAD_MAX_SLOTS; i++) {
        if (g_slots[i].active) {
            return vpad_send_data_slot(i, data);
        }
    }
    return -1;
}

void vpad_set_rumble_callback(vpad_rumble_cb_t cb) {
    g_rumble_cb = cb;
}

void vpad_poll_rumble(void) {
    uint64_t now_pulse = daemon_sys_time_us();
    for (int i = 0; i < VPAD_MAX_SLOTS; i++) {
        if (!g_slots[i].active || g_slots[i].handle < 0) continue;

        if (g_slots[i].ps_pulse_active) {
            if (now_pulse >= g_slots[i].ps_pulse_until_us) {
                g_slots[i].ps_pulse_active = false;
                if (p_scePadVirtualDeviceInsertData) {
                    OrbisPadData rel = g_slots[i].last_sent_data;
                    rel.buttons &= ~DS4_BUTTON_PS;
                    rel.connected = 1;
                    rel.count = 1;
                    rel.timestamp = now_pulse;
                    p_scePadVirtualDeviceInsertData(g_slots[i].handle, &rel);
                    g_slots[i].last_send_time_us = now_pulse;
                    g_slots[i].last_sent_data = rel;
                }
                daemon_log(LOG_LEVEL_INFO, "vpad_pulse_ps_button_slot(%d): PS button pulse complete (via poll_rumble)", i);
            } else if (now_pulse >= g_slots[i].ps_pulse_start_us &&
                       (now_pulse - g_slots[i].last_send_time_us) >= 16000ULL &&
                       p_scePadVirtualDeviceInsertData) {
                OrbisPadData hold = g_slots[i].last_sent_data;
                hold.buttons |= DS4_BUTTON_PS;
                hold.connected = 1;
                hold.count = 1;
                hold.timestamp = now_pulse;
                p_scePadVirtualDeviceInsertData(g_slots[i].handle, &hold);
                g_slots[i].last_send_time_us = now_pulse;
                g_slots[i].last_sent_data = hold;
            }
        }

        if (g_slots[i].ps_followup_pending && now_pulse >= g_slots[i].ps_followup_at_us) {
            g_slots[i].ps_followup_pending = false;
            // Fire fallback follow-up pulse only if user didn't interact and no new user logged in
            if (!g_slots[i].user_interacted &&
                ((i >= 1 && s_last_login_count <= g_slots[i].slot_open_login_count) || s_last_login_count <= 0)) {
                daemon_log(LOG_LEVEL_INFO, "vpad_poll_rumble: Firing follow-up PS pulse for slot %d after settle", i);
                vpad_schedule_ps_pulse_slot(i, 0, 160000ULL);
            }
        }
    }

    if (!p_scePadVirtualDeviceGetRemoteSetting) return;

    for (int i = 0; i < VPAD_MAX_SLOTS; i++) {
        if (!g_slots[i].active || g_slots[i].handle < 0) continue;
        if (g_slots[i].last_remote_poll_us != 0 && (now_pulse - g_slots[i].last_remote_poll_us) < 8000ULL) {
            continue;
        }
        g_slots[i].last_remote_poll_us = now_pulse;

        uint8_t setting[128];
        memset(setting, 0, sizeof(setting));
        int ret = p_scePadVirtualDeviceGetRemoteSetting(g_slots[i].handle, setting);
        if (ret == 0) {
            uint8_t lg = setting[3];
            uint8_t sm = setting[4];
            uint8_t cr = setting[5];
            uint8_t cg = setting[6];
            uint8_t cb = setting[7];
            uint64_t now = daemon_sys_time_us();

            // Track raw setting changes from libScePad to guard against stale non-zero rumble
            // left in the GetRemoteSetting buffer when a game is closed or suspended via the PS button.
            if (lg != g_slots[i].last_raw_rumble_lg || sm != g_slots[i].last_raw_rumble_sm) {
                g_slots[i].last_raw_rumble_lg = lg;
                g_slots[i].last_raw_rumble_sm = sm;
                g_slots[i].last_rumble_change_us = now;
            }
            if (g_slots[i].last_ps_state == 1 ||
                ((lg > 0 || sm > 0) && g_slots[i].last_rumble_change_us != 0 &&
                 (now - g_slots[i].last_rumble_change_us >= 4000000ULL))) {
                lg = 0;
                sm = 0;
            }

            if (lg > 0 || sm > 0) {
                g_slots[i].stop_retries_remaining = 0;
            }

            bool rumble_changed = (lg != g_slots[i].last_rumble_lg || sm != g_slots[i].last_rumble_sm);
            bool color_changed = (g_slots[i].source == VPAD_SOURCE_BT &&
                                 g_daemon_config.dualsense_lightbar_mode == 0 &&
                                 (cr != g_slots[i].last_color_r || cg != g_slots[i].last_color_g || cb != g_slots[i].last_color_b));
            bool keepalive = ((lg > 0 || sm > 0) && (now - g_slots[i].last_rumble_send_us >= 100000ULL));

            // Stop condition (lg=0, sm=0): send immediately so vibration cuts off without latency.
            // When stopping, trigger up to 3 additional repeats spaced by 50ms so a dropped packet
            // or busy transfer never leaves the motors vibrating indefinitely.
            bool should_send = false;
            if (rumble_changed) {
                if (lg == 0 && sm == 0) {
                    should_send = true;
                    g_slots[i].stop_retries_remaining = 3;
                } else if (now - g_slots[i].last_rumble_send_us >= 20000ULL) {
                    should_send = true;
                }
            } else if (lg == 0 && sm == 0 && g_slots[i].stop_retries_remaining > 0) {
                if (now - g_slots[i].last_rumble_send_us >= 50000ULL) {
                    should_send = true;
                    g_slots[i].stop_retries_remaining--;
                }
            } else if (color_changed) {
                if (now - g_slots[i].last_rumble_send_us >= 20000ULL) {
                    should_send = true;
                }
            } else if (keepalive) {
                should_send = true;
            }

            if (should_send) {
                g_slots[i].last_rumble_lg = lg;
                g_slots[i].last_rumble_sm = sm;
                g_slots[i].last_color_r = cr;
                g_slots[i].last_color_g = cg;
                g_slots[i].last_color_b = cb;
                g_slots[i].last_rumble_send_us = now;

                if (g_slots[i].source == VPAD_SOURCE_BT) {
                    if (g_daemon_config.enable_rumble || (lg == 0 && sm == 0) || color_changed) {
                        uint8_t out_lg = g_daemon_config.enable_rumble ? lg : 0;
                        uint8_t out_sm = g_daemon_config.enable_rumble ? sm : 0;
                        uint8_t color[3] = {
                            cr > 63 ? 255 : (uint8_t)(cr * 4),
                            cg > 63 ? 255 : (uint8_t)(cg * 4),
                            cb > 63 ? 255 : (uint8_t)(cb * 4)
                        };
                        bt_driver_send_rumble_slot(i, out_lg, out_sm, color);
                    }
                } else if (g_slots[i].source == VPAD_SOURCE_USB) {
                    if (g_daemon_config.enable_rumble || (lg == 0 && sm == 0)) {
                        if (g_rumble_cb) {
                            g_rumble_cb(lg, sm);
                        } else {
                            usb_driver_send_rumble(lg, sm);
                        }
                    }
                }
            }
        }
    }
}
