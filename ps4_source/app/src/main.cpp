#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <orbis/libkernel.h>
#include <orbis/Pad.h>
#include <orbis/UserService.h>
#include <orbis/SystemService.h>

#include "common/graphics.h"
#include "common/log.h"
#include "config_manager.h"
#include "installer.h"
#include "ui.h"
#include "hid_profile.h"
#include "pkg_updater.h"
#include "onefamily_version.h"

static bool check_running_daemon_version(const char* expectedVer) {
    const char* paths[2] = {
        "/data/anycontroller/daemon.version",
        "/user/data/anycontroller/daemon.version"
    };
    for (int i = 0; i < 2; i++) {
        int fd = sceKernelOpen(paths[i], O_RDONLY, 0);
        if (fd >= 0) {
            char buf[64];
            memset(buf, 0, sizeof(buf));
            ssize_t rd = sceKernelRead(fd, buf, sizeof(buf) - 1);
            sceKernelClose(fd);
            if (rd > 0) {
                buf[rd] = '\0';
                char* p = buf;
                while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
                size_t len = strlen(p);
                while (len > 0 && (p[len - 1] == ' ' || p[len - 1] == '\t' || p[len - 1] == '\r' || p[len - 1] == '\n')) {
                    p[--len] = '\0';
                }
                if (strcmp(p, expectedVer) == 0) {
                    return true;
                }
            }
        }
    }
    return false;
}

static void notify_daemon_reload() {
    const char* reload_paths[2] = {
        "/data/anycontroller/anycontroller.reload",
        "/user/data/anycontroller/anycontroller.reload"
    };
    for (int i = 0; i < 2; i++) {
        int rfd = sceKernelOpen(reload_paths[i], O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (rfd >= 0) {
            sceKernelWrite(rfd, "1\n", 2);
            sceKernelClose(rfd);
        }
    }
}

static void send_vibe_to_daemon(uint8_t lgMotor, uint8_t smMotor, uint8_t slot = 0) {
    static uint8_t s_last_sent_lg = 0;
    static uint8_t s_last_sent_sm = 0;
    static uint8_t s_last_sent_slot = 0;
    static bool s_has_sent = false;

    if (slot > 3) slot = 0;
    if (s_has_sent && lgMotor == s_last_sent_lg && smMotor == s_last_sent_sm && slot == s_last_sent_slot) {
        return; // State unchanged, avoid spamming filesystem I/O on 60fps render loop
    }
    s_last_sent_lg = lgMotor;
    s_last_sent_sm = smMotor;
    s_last_sent_slot = slot;
    s_has_sent = true;

    const char* vibe_paths[2] = {
        "/data/anycontroller/anycontroller.vibe",
        "/user/data/anycontroller/anycontroller.vibe"
    };

    if (lgMotor == 0 && smMotor == 0) {
        for (int i = 0; i < 2; i++) {
            sceKernelUnlink(vibe_paths[i]);
        }
        s_has_sent = false;
    } else {
        mkdir("/data", 0777);
        mkdir("/data/anycontroller", 0777);
        mkdir("/user", 0777);
        mkdir("/user/data", 0777);
        mkdir("/user/data/anycontroller", 0777);
        uint8_t data[3] = { lgMotor, smMotor, slot };
        for (int i = 0; i < 2; i++) {
            int fd = sceKernelOpen(vibe_paths[i], O_WRONLY | O_CREAT | O_TRUNC, 0666);
            if (fd >= 0) {
                sceKernelWrite(fd, data, sizeof(data));
                sceKernelClose(fd);
            }
        }
    }
}

static void request_daemon_rebind_profile() {
    const char* rebind_paths[2] = {
        "/data/anycontroller/anycontroller.rebind",
        "/user/data/anycontroller/anycontroller.rebind"
    };
    for (int i = 0; i < 2; i++) {
        int rfd = sceKernelOpen(rebind_paths[i], O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (rfd >= 0) {
            sceKernelWrite(rfd, "-1\n", 3);
            sceKernelClose(rfd);
        }
    }
}

typedef int (*scePadSetLightBarBaseBrightness_t)(int handle, int brightness);
typedef int (*sceUserServiceSetLightBarBaseBrightness_t)(int userId, int brightness);

static AppConfig s_slotDs4Cfgs[4];
static bool s_slotDs4Dirty = true;

static void sync_slot_ds4_configs(const AppConfig& activeCfg, const std::string& activeTargetId, bool force) {
    if (!force && !s_slotDs4Dirty) return;
    for (int i = 0; i < 4; i++) {
        char idBuf[24];
        snprintf(idBuf, sizeof(idBuf), "ds4_slot_%d", i + 1);
        if (activeTargetId == idBuf) {
            s_slotDs4Cfgs[i] = activeCfg;
        } else {
            ConfigManager tmp;
            tmp.loadTarget(idBuf);
            s_slotDs4Cfgs[i] = tmp.getConfig();
        }
    }
    s_slotDs4Dirty = false;
}

static void apply_live_pad_lightbar(int padHandles[4], const bool btClaimedSlots[4], const AppConfig slotCfgs[4]) {
    static bool s_resolved = false;
    static scePadSetLightBarBaseBrightness_t s_padSetBaseBr = NULL;
    static sceUserServiceSetLightBarBaseBrightness_t s_userSetBaseBr = NULL;
    if (!s_resolved) {
        s_resolved = true;
        int padMod = sceKernelLoadStartModule("libScePad.sprx", 0, NULL, 0, NULL, NULL);
        if (padMod >= 0) {
            sceKernelDlsym(padMod, "scePadSetLightBarBaseBrightness", (void**)&s_padSetBaseBr);
        }
        int usrMod = sceKernelLoadStartModule("libSceUserService.sprx", 0, NULL, 0, NULL, NULL);
        if (usrMod >= 0) {
            sceKernelDlsym(usrMod, "sceUserServiceSetLightBarBaseBrightness", (void**)&s_userSetBaseBr);
        }
    }

    static const uint8_t ds4_colors[7][3] = {
        {0, 112, 220},   // 0: Azul PS4
        {239, 68, 68},   // 1: Rojo Carmesi
        {16, 185, 129},  // 2: Verde Esmeralda
        {168, 85, 247},  // 3: Purpura Neon
        {6, 182, 212},   // 4: Cian Electrico
        {245, 158, 11},  // 5: Ambar Oro
        {248, 250, 252}  // 6: Blanco Puro
    };

    uint64_t now_us = sceKernelGetProcessTime();

    OrbisUserServiceLoginUserIdList userList;
    memset(&userList, 0, sizeof(userList));
    bool hasUsers = (sceUserServiceGetLoginUserIdList(&userList) == 0);

    for (int i = 0; i < 4; i++) {
        if (padHandles[i] < 0) continue;

        // El halo RGB del DualSense PS5 en esta ranura lo controla directamente el demonio via reporte BT 0x31
        if (btClaimedSlots && btClaimedSlots[i]) {
            continue;
        }

        const AppConfig& cfg = slotCfgs[i];
        int br = cfg.ds4_brightness;
        if (br < 0 || br > 2) br = 0;
        int hw_base_br = (cfg.ds4_lightbar_mode == 3) ? 2 : br;

        if (s_padSetBaseBr) {
            s_padSetBaseBr(padHandles[i], hw_base_br);
        }
        if (s_userSetBaseBr && hasUsers && userList.userId[i] != ORBIS_USER_SERVICE_USER_ID_INVALID) {
            s_userSetBaseBr(userList.userId[i], hw_base_br);
        }

        if (cfg.ds4_lightbar_mode == 0 && br == 0) {
            scePadResetLightBar(padHandles[i]);
            continue;
        }
        // CRITICAL: Sony's libScePad requires the 4th byte (reserve[0]) of OrbisPadColor to be 0!
        OrbisPadColor col = { 0, 0, 0, 0 };
        int idx = cfg.ds4_lightbar_color;
        if (idx < 0 || idx > 6) idx = 0;

        if (cfg.ds4_lightbar_mode == 3) {
            // Mode 3: Sin color / Apagado Completo (0% LED)
            col.r = 0; col.g = 0; col.b = 0; col.a = 0;
            if (scePadSetLightBar(padHandles[i], &col) < 0) {
                OrbisPadColor dim_off = { 1, 0, 0, 0 };
                scePadSetLightBar(padHandles[i], &dim_off);
            }
            continue;
        } else if (cfg.ds4_lightbar_mode == 0) {
            col.r = 0; col.g = 112; col.b = 220;
        } else if (cfg.ds4_lightbar_mode == 2) {
            col.r = 16; col.g = 220; col.b = 110;
        } else if (cfg.ds4_lightbar_mode == 4) {
            // Dynamic Rainbow Cycle (~5s full spectrum)
            uint32_t phase = (uint32_t)((now_us / 3200ULL) % 1536ULL);
            uint32_t region = phase / 256U;
            uint32_t rem    = phase % 256U;
            switch (region) {
                case 0:  col.r = 255;                col.g = (uint8_t)rem;       col.b = 0; break;
                case 1:  col.r = (uint8_t)(255-rem); col.g = 255;                col.b = 0; break;
                case 2:  col.r = 0;                  col.g = 255;                col.b = (uint8_t)rem; break;
                case 3:  col.r = 0;                  col.g = (uint8_t)(255-rem); col.b = 255; break;
                case 4:  col.r = (uint8_t)rem;       col.g = 0;                  col.b = 255; break;
                default: col.r = 255;                col.g = 0;                  col.b = (uint8_t)(255-rem); break;
            }
        } else if (cfg.ds4_lightbar_mode == 5) {
            // Breathing Pulse (~2.8s cycle)
            uint32_t phase = (uint32_t)((now_us / 5500ULL) % 512ULL);
            uint32_t tri = (phase < 256U) ? phase : (511U - phase);
            uint32_t pulse_pct = 8U + (tri * 92U) / 255U;
            col.r = (uint8_t)((ds4_colors[idx][0] * pulse_pct) / 100U);
            col.g = (uint8_t)((ds4_colors[idx][1] * pulse_pct) / 100U);
            col.b = (uint8_t)((ds4_colors[idx][2] * pulse_pct) / 100U);
        } else {
            col.r = ds4_colors[idx][0];
            col.g = ds4_colors[idx][1];
            col.b = ds4_colors[idx][2];
        }

        if (br == 1) {
            col.r = (uint8_t)((col.r * 25) / 100);
            col.g = (uint8_t)((col.g * 25) / 100);
            col.b = (uint8_t)((col.b * 25) / 100);
        } else if (br == 2) {
            uint8_t orig_r = col.r, orig_g = col.g, orig_b = col.b;
            col.r = (uint8_t)((col.r * 5) / 100);
            col.g = (uint8_t)((col.g * 5) / 100);
            col.b = (uint8_t)((col.b * 5) / 100);
            if (orig_r > 0 && col.r == 0) col.r = 1;
            if (orig_g > 0 && col.g == 0) col.g = 1;
            if (orig_b > 0 && col.b == 0) col.b = 1;
        }
        scePadSetLightBar(padHandles[i], &col);
    }
}

static void refresh_pad_handles(int padHandles[4], int initialUserId) {
    OrbisUserServiceLoginUserIdList userList;
    memset(&userList, 0, sizeof(userList));
    bool hasUserList = (sceUserServiceGetLoginUserIdList(&userList) == 0);

    for (int i = 0; i < 4; i++) {
        // If handle is already valid and active, verify if it is alive
        if (padHandles[i] >= 0) {
            OrbisPadData testData;
            memset(&testData, 0, sizeof(testData));
            if (scePadReadState(padHandles[i], &testData) == 0 && testData.connected) {
                continue;
            }
            scePadClose(padHandles[i]);
            padHandles[i] = -1;
        }

        // Open controller for user slot i if that user is logged in
        if (hasUserList && userList.userId[i] != ORBIS_USER_SERVICE_USER_ID_INVALID) {
            padHandles[i] = scePadOpen(userList.userId[i], 0, 0, NULL);
        } else if (!hasUserList && i == 0 && initialUserId >= 0) {
            // Fallback strictly for Player 1 only if user listing failed
            padHandles[0] = scePadOpen(initialUserId, 0, 0, NULL);
        }
    }
}

int main() {
    setvbuf(stdout, NULL, _IONBF, 0);

    // Initialize 2D Graphics (1920x1080 @ 60 Hz)
    Scene2D* scene = new Scene2D(1920, 1080, 4);
    if (!scene->Init(0xC000000, 2)) {
        return -1;
    }

    // Purge any legacy onefamily files/directories on every launch
    Installer::cleanupLegacyOneFamily();

    // Initialize Config Manager
    // NOTE: If /data/anycontroller/anycontroller.ini does not exist,
    // configMgr.load() automatically creates it with all default settings!
    ConfigManager configMgr;
    configMgr.load();
    notify_daemon_reload();

    // Initialize UI
    UI* ui = new UI();
    if (!ui->init(scene)) {
        return -1;
    }

    // Initialize Controller via OpenOrbis scePad
    scePadInit();
    int userId = -1;
    OrbisUserServiceInitializeParams uParam;
    uParam.priority = ORBIS_KERNEL_PRIO_FIFO_LOWEST;
    sceUserServiceInitialize(&uParam);
    sceUserServiceGetInitialUser(&userId);

    // Standard handles for up to 4 player controller slots
    int padHandles[4];
    for (int i = 0; i < 4; i++) padHandles[i] = -1;
    refresh_pad_handles(padHandles, userId);
    {
        HidStatus initHid;
        HidProfile::read_status(initHid);
        bool initBtClaimed[4] = { false, false, false, false };
        for (size_t d = 0; d < initHid.devices.size(); d++) {
            int ps = initHid.devices[d].player_slot;
            if (ps >= 1 && ps <= 4) initBtClaimed[ps - 1] = true;
        }
        sync_slot_ds4_configs(configMgr.getConfig(), "", true);
        apply_live_pad_lightbar(padHandles, initBtClaimed, s_slotDs4Cfgs);
    }

    // Check if First-Launch Setup Wizard or Payload Update is required
    bool isInstalled = Installer::isInstalled();
    bool isUpToDate = Installer::isPayloadUpToDate();

    if (!isInstalled || !isUpToDate) {
        bool isUpdate = isInstalled && !isUpToDate;
        InstallResult res = Installer::runInstallation(isUpdate);
        notify_daemon_reload();

        int wizardFrames = 0;
        uint32_t wizardPrevButtons = 0;
        bool exitAfterWizard = false;

        for (;;) {
            uint32_t anyWizardButtons = 0;
            for (int i = 0; i < 4; i++) {
                if (padHandles[i] >= 0) {
                    OrbisPadData samples[16];
                    memset(samples, 0, sizeof(samples));
                    int n = scePadRead(padHandles[i], samples, 16);
                    if (n > 0) {
                        for (int s = 0; s < n; s++) {
                            if (samples[s].connected) anyWizardButtons |= samples[s].buttons;
                        }
                    } else {
                        OrbisPadData d;
                        memset(&d, 0, sizeof(d));
                        if (scePadReadState(padHandles[i], &d) == 0 && d.connected) {
                            anyWizardButtons |= d.buttons;
                        }
                    }
                }
            }

            uint32_t curButtons = anyWizardButtons;
            uint32_t justPressed = curButtons & ~wizardPrevButtons;
            wizardPrevButtons = curButtons;

            // Continue into app on CROSS (X), or exit to PS4 shell on CIRCLE / OPTIONS
            if (justPressed & ORBIS_PAD_BUTTON_CROSS) {
                exitAfterWizard = false;
                break;
            }
            if (justPressed & (ORBIS_PAD_BUTTON_CIRCLE | ORBIS_PAD_BUTTON_OPTIONS)) {
                exitAfterWizard = true;
                break;
            }

            ui->render_wizard(scene, res, wizardFrames);

            scene->SubmitFlip(wizardFrames);
            scene->FrameWait(wizardFrames);
            scene->FrameBufferSwap();
            wizardFrames++;
        }

        if (exitAfterWizard) {
            for (int i = 0; i < 4; i++) {
                if (padHandles[i] >= 0) {
                    scePadClose(padHandles[i]);
                }
            }
            sceSystemServiceLoadExec("exit", NULL);
            exit(0);
            return 0;
        }
    }

    int currentTabId = TAB_SETTINGS;
    int selectedCard = 0;
    int dsSelectedCard = 0;
    int ds4SelectedCard = 0;
    int toastFrames = 0;
    std::string toastText = "Ajustes guardados en /data/anycontroller/anycontroller.ini";
    int frameID = 0;
    uint32_t prevButtons = 0;
    TesterState tester;
    std::string activeTargetId = "";

    HidStatus hidStatus;
    HidProfile::read_status(hidStatus);
    HidProfile hidProfile;
    bool hidLoaded = false;
    int remapSelectedRow = 0;
    bool remapTestMode = false;

    // Start background check for online PKG updates
    PkgUpdater& updater = PkgUpdater::instance();
    updater.start_check();
    UpdaterState lastUpdaterState = UPDATER_IDLE;

    // Warn if the GoldHEN daemon loaded in PS4 RAM is not yet this exact version
    if (!check_running_daemon_version(ONEFAMILY_VERSION_STRING)) {
        toastText = std::string("Reinicia la PS4 1 vez para activar el nuevo demonio ") + ONEFAMILY_VERSION_STRING + " en memoria";
        toastFrames = 360;
    }

    // Main Interaction Loop
    for (;;) {
        // Check if daemon requested physical DS4 disconnect upon PS5 DualSense connection
        int req_fd = sceKernelOpen("/data/anycontroller/ds4_disconnect.req", O_RDONLY, 0);
        if (req_fd >= 0) {
            sceKernelClose(req_fd);
            sceKernelUnlink("/data/anycontroller/ds4_disconnect.req");
            for (int i = 0; i < 4; i++) {
                if (padHandles[i] >= 0) {
                    reinterpret_cast<int(*)(int)>(scePadDisconnectPort)(padHandles[i]);
                    scePadClose(padHandles[i]);
                    padHandles[i] = -1;
                }
            }
            usleep(120000);
            refresh_pad_handles(padHandles, userId);
            HidProfile::read_status(hidStatus);
        }

        // Periodically refresh controller handles to support dynamic reconnects and multi-user login
        if (frameID % 30 == 0) {
            refresh_pad_handles(padHandles, userId);
        }

        OrbisPadData padDataSlots[4];
        uint32_t anyButtons = 0;
        for (int i = 0; i < 4; i++) {
            memset(&padDataSlots[i], 0, sizeof(padDataSlots[i]));
            if (padHandles[i] >= 0) {
                OrbisPadData samples[16];
                memset(samples, 0, sizeof(samples));
                int n = scePadRead(padHandles[i], samples, 16);
                if (n > 0) {
                    for (int s = 0; s < n; s++) {
                        if (samples[s].connected) {
                            anyButtons |= samples[s].buttons;
                        }
                    }
                    padDataSlots[i] = samples[n - 1];
                    tester.slotConnected[i] = samples[n - 1].connected;
                } else if (scePadReadState(padHandles[i], &padDataSlots[i]) == 0 && padDataSlots[i].connected) {
                    anyButtons |= padDataSlots[i].buttons;
                    tester.slotConnected[i] = true;
                } else {
                    tester.slotConnected[i] = false;
                }
            } else {
                tester.slotConnected[i] = false;
            }
        }

        // Mantener manualmente la ranura elegida sin saltos automáticos entre jugadores;
        // solo hacer fallback si la ranura activa ya no está conectada.
        if (tester.selectedSlot < 0 || tester.selectedSlot >= 4 || !tester.slotConnected[tester.selectedSlot]) {
            for (int i = 0; i < 4; i++) {
                if (tester.slotConnected[i]) {
                    tester.selectedSlot = i;
                    break;
                }
            }
        }
        if (tester.selectedSlot < 0 || tester.selectedSlot >= 4) {
            tester.selectedSlot = 0;
        }
        OrbisPadData padData = padDataSlots[tester.selectedSlot];

        // Left Analog Stick Navigation Emulation (Stick-to-DPad with Auto-Repeat)
        static int s_stickRepeatFrames = 0;
        static uint32_t s_stickNavButtons = 0;

        uint32_t stickNav = 0;
        const int STICK_NAV_THRESHOLD = 64; // < 64 or > 192 from center 128
        if (padData.leftStick.y < 128 - STICK_NAV_THRESHOLD) stickNav |= ORBIS_PAD_BUTTON_UP;
        else if (padData.leftStick.y > 128 + STICK_NAV_THRESHOLD) stickNav |= ORBIS_PAD_BUTTON_DOWN;

        if (padData.leftStick.x < 128 - STICK_NAV_THRESHOLD) stickNav |= ORBIS_PAD_BUTTON_LEFT;
        else if (padData.leftStick.x > 128 + STICK_NAV_THRESHOLD) stickNav |= ORBIS_PAD_BUTTON_RIGHT;

        uint32_t stickJustPressed = 0;
        // Suppress stick navigation emulation during Remap Test Mode or Tester Test Mode
        if (remapTestMode || tester.testMode) {
            s_stickNavButtons = 0;
            s_stickRepeatFrames = 0;
        } else if (stickNav != 0) {
            if (stickNav != s_stickNavButtons) {
                stickJustPressed = stickNav;
                s_stickNavButtons = stickNav;
                s_stickRepeatFrames = 20; // ~330ms delay
            } else {
                if (s_stickRepeatFrames > 0) {
                    s_stickRepeatFrames--;
                } else {
                    stickJustPressed = stickNav;
                    s_stickRepeatFrames = 8; // ~130ms repeat rate
                }
            }
        } else {
            s_stickNavButtons = 0;
            s_stickRepeatFrames = 0;
        }

        uint32_t curButtons = anyButtons;
        uint32_t justPressed = (curButtons & ~prevButtons) | stickJustPressed;
        prevButtons = curButtons;

        // Poll HID / DualSense controller status every 10 frames (6 Hz) for fast detection & live battery updates
        if (frameID % 10 == 0) {
            HidProfile::read_status(hidStatus);
        }
        bool isHidType = (hidStatus.type == "generic_hid" ||
                          hidStatus.type == "Generic HID Gamepad" ||
                          hidStatus.type.find("hid") != std::string::npos ||
                          hidStatus.type.find("HID") != std::string::npos);
        bool hasHid = (hidStatus.connected && isHidType);

        // Detect whether a PS5 DualSense controller is currently connected via ANYCONTROLLER daemon
        bool isDualSenseActive = (hidStatus.connected &&
                                  (hidStatus.type == "dualsense" ||
                                   hidStatus.type == "dualsense_bt" ||
                                   (hidStatus.vid == 0x054C && (hidStatus.pid == 0x0CE6 || hidStatus.pid == 0x0DF2))));
        // Keep both DualSense PS5 and DualShock 4 tabs available in sidebar so user is never locked out,
        // and auto-switch between them whenever a DualSense connects or disconnects!
        bool hasDualSense = true;

        // Build per-controller target list (Index 0 = Global, followed by BT DualSense by MAC & native DS4 slots)
        bool btClaimedSlots[4] = { false, false, false, false };
        std::vector<ControllerTarget> targets;
        {
            ControllerTarget globalT;
            globalT.id = "";
            globalT.shortLabel = "Global";
            globalT.detailLabel = "Global (Base por defecto / anycontroller.ini)";
            globalT.isDualSense = isDualSenseActive;
            globalT.slotIndex = -1;
            globalT.hasCustom = false;
            targets.push_back(globalT);

            if (isDualSenseActive) {
                for (size_t d = 0; d < hidStatus.devices.size(); d++) {
                    const DualSenseDeviceInfo& dev = hidStatus.devices[d];
                    int ps = (dev.player_slot >= 1 && dev.player_slot <= 4) ? dev.player_slot : ((int)d + 1);
                    if (ps < 1) ps = 1;
                    if (ps > 4) ps = 4;
                    int sIdx = ps - 1;
                    btClaimedSlots[sIdx] = true;

                    std::string macHex;
                    for (size_t c = 0; c < dev.mac.size(); c++) {
                        char ch = dev.mac[c];
                        if (ch >= 'a' && ch <= 'f') ch = (char)(ch - 'a' + 'A');
                        if ((ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'F')) {
                            macHex.push_back(ch);
                        }
                    }
                    if (!macHex.empty()) {
                        ControllerTarget dsT;
                        dsT.id = std::string("ds_mac_") + macHex;
                        dsT.isDualSense = true;
                        dsT.slotIndex = sIdx;
                        dsT.hasCustom = configMgr.hasCustomTarget(dsT.id);
                        dsT.batteryPct = dev.battery_pct;
                        dsT.charging = dev.battery_charging;
                        char sBuf[48], dBuf[160];
                        snprintf(sBuf, sizeof(sBuf), "J%d DualSense", ps);
                        std::string tailMac = (macHex.size() > 4) ? macHex.substr(macHex.size() - 4) : macHex;
                        snprintf(dBuf, sizeof(dBuf), "J%d • DualSense PS5 [%s] (%s.ini)", ps, tailMac.c_str(), dsT.id.c_str());
                        dsT.shortLabel = sBuf;
                        dsT.detailLabel = dBuf;
                        targets.push_back(dsT);
                    }
                }
            }

            for (int s = 0; s < 4; s++) {
                if (tester.slotConnected[s] && !btClaimedSlots[s]) {
                    ControllerTarget ds4T;
                    char idBuf[24], sBuf[48], dBuf[160];
                    snprintf(idBuf, sizeof(idBuf), "ds4_slot_%d", s + 1);
                    snprintf(sBuf, sizeof(sBuf), "J%d DS4", s + 1);
                    snprintf(dBuf, sizeof(dBuf), "J%d • DualShock 4 Nativo (%s.ini)", s + 1, idBuf);
                    ds4T.id = idBuf;
                    ds4T.shortLabel = sBuf;
                    ds4T.detailLabel = dBuf;
                    ds4T.isDualSense = false;
                    ds4T.slotIndex = s;
                    ds4T.hasCustom = configMgr.hasCustomTarget(ds4T.id);
                    targets.push_back(ds4T);
                }
            }
        }

        int activeTargetIndex = -1;
        for (size_t ti = 0; ti < targets.size(); ti++) {
            if (targets[ti].id == activeTargetId) {
                activeTargetIndex = (int)ti;
                break;
            }
        }
        if (activeTargetIndex < 0) {
            activeTargetId = "";
            activeTargetIndex = 0;
            configMgr.loadTarget("");
            s_slotDs4Dirty = true;
        }

        // Live lightbar sync per independent DS4 slot (never touching DualSense BT slots)
        bool anyDs4Animated = false;
        for (int s = 0; s < 4; s++) {
            if (tester.slotConnected[s] && !btClaimedSlots[s] &&
                (s_slotDs4Cfgs[s].ds4_lightbar_mode == 4 || s_slotDs4Cfgs[s].ds4_lightbar_mode == 5)) {
                anyDs4Animated = true;
                break;
            }
        }
        if (frameID % 30 == 0) {
            sync_slot_ds4_configs(configMgr.getConfig(), activeTargetId, true);
            apply_live_pad_lightbar(padHandles, btClaimedSlots, s_slotDs4Cfgs);
        } else if (anyDs4Animated && (frameID % 3 == 0)) {
            apply_live_pad_lightbar(padHandles, btClaimedSlots, s_slotDs4Cfgs);
        }

        static int s_prevDualSenseActive = -1;
        if (s_prevDualSenseActive == -1) {
            s_prevDualSenseActive = isDualSenseActive ? 1 : 0;
        } else if ((isDualSenseActive ? 1 : 0) != s_prevDualSenseActive) {
            s_prevDualSenseActive = isDualSenseActive ? 1 : 0;
            if (isDualSenseActive && currentTabId == TAB_DUALSHOCK4) {
                currentTabId = TAB_DUALSENSE;
                toastText = "Mando PS5 DualSense detectado: cambiado a pestaña DualSense PS5";
                toastFrames = 150;
            } else if (!isDualSenseActive && currentTabId == TAB_DUALSENSE) {
                currentTabId = TAB_DUALSHOCK4;
                toastText = "Usando mando PS4 DualShock 4: cambiado a pestaña DualShock 4";
                toastFrames = 150;
            }
        }

        if (hasHid && !hidLoaded) {
            hidProfile.load(hidStatus.vid, hidStatus.pid, hidStatus.nbuttons);
            hidLoaded = true;
        } else if (!hasHid) {
            hidLoaded = false;
        }

        // Build active tab list (both DualSense PS5 and DualShock 4 always accessible via L1/R1)
        std::vector<int> activeTabs;
        activeTabs.push_back(TAB_SETTINGS);
        if (isDualSenseActive) {
            activeTabs.push_back(TAB_DUALSENSE);
            activeTabs.push_back(TAB_DUALSHOCK4);
        } else {
            activeTabs.push_back(TAB_DUALSHOCK4);
            activeTabs.push_back(TAB_DUALSENSE);
        }
        activeTabs.push_back(TAB_TESTER);
        if (hasHid) {
            activeTabs.push_back(TAB_REMAP_HID);
        }
        activeTabs.push_back(TAB_INFO);

        if (currentTabId == TAB_REMAP_HID && !hasHid) {
            currentTabId = TAB_SETTINGS;
        }

        int numTabs = (int)activeTabs.size();
        int activeTabIndex = 0;
        for (int t = 0; t < numTabs; t++) {
            if (activeTabs[t] == currentTabId) {
                activeTabIndex = t;
                break;
            }
        }
        currentTabId = activeTabs[activeTabIndex];

        AppConfig& cfg = configMgr.getConfig();

        auto saveAndApplyActiveTarget = [&]() -> bool {
            bool saved = configMgr.saveTarget(activeTargetId);
            if (!activeTargetId.empty()) {
                targets[activeTargetIndex].hasCustom = true;
            }
            s_slotDs4Dirty = true;
            notify_daemon_reload();
            sync_slot_ds4_configs(cfg, activeTargetId, true);
            apply_live_pad_lightbar(padHandles, btClaimedSlots, s_slotDs4Cfgs);
            return saved;
        };

        // Manual Controller Target Switching (L3) & Restore to Global (R3) across Config Tabs
        if (currentTabId == TAB_SETTINGS || currentTabId == TAB_DUALSENSE || currentTabId == TAB_DUALSHOCK4) {
            if (justPressed & ORBIS_PAD_BUTTON_L3) {
                if (targets.size() > 1) {
                    activeTargetIndex = (activeTargetIndex + 1) % (int)targets.size();
                    activeTargetId = targets[activeTargetIndex].id;
                    configMgr.loadTarget(activeTargetId);
                    if (targets[activeTargetIndex].slotIndex >= 0) {
                        tester.selectedSlot = targets[activeTargetIndex].slotIndex;
                    }
                    s_slotDs4Dirty = true;
                    if (activeTargetId.empty()) {
                        toastText = "Editando: Base Global (los mandos sin perfil propio heredan esto)";
                    } else {
                        toastText = std::string("Editando: ") + targets[activeTargetIndex].shortLabel +
                                    (targets[activeTargetIndex].hasCustom ? " (memoria propia cargada)" : " (heredando Global)");
                    }
                    toastFrames = 150;
                } else {
                    toastText = "Solo hay un perfil activo (Global). Conecta 1 o más mandos para editar por separado.";
                    toastFrames = 150;
                }
            }
            if (justPressed & ORBIS_PAD_BUTTON_R3) {
                if (!activeTargetId.empty() && configMgr.hasCustomTarget(activeTargetId)) {
                    configMgr.clearTarget(activeTargetId);
                    targets[activeTargetIndex].hasCustom = false;
                    s_slotDs4Dirty = true;
                    notify_daemon_reload();
                    sync_slot_ds4_configs(cfg, activeTargetId, true);
                    apply_live_pad_lightbar(padHandles, btClaimedSlots, s_slotDs4Cfgs);
                    toastText = "Mando restaurado: ahora hereda Ajustes Globales";
                    toastFrames = 180;
                } else if (!activeTargetId.empty()) {
                    toastText = "Este mando ya está heredando la configuración Global";
                    toastFrames = 120;
                }
            }
        }

        // Exit ONLY on OPTIONS (CIRCLE returns to TAB_SETTINGS instead of closing the app!)
        if (!remapTestMode && !tester.testMode) {
            if (justPressed & ORBIS_PAD_BUTTON_OPTIONS) {
                break;
            }
            if ((justPressed & ORBIS_PAD_BUTTON_CIRCLE) && currentTabId != TAB_SETTINGS) {
                currentTabId = TAB_SETTINGS;
                if (tester.rumbleFrames > 0 || tester.rumblePhase != 0) {
                    tester.rumbleFrames = 0;
                    tester.rumblePhase = 0;
                    OrbisPadVibeParam stopVibe = { 0, 0 };
                    for (int i = 0; i < 4; i++) {
                        if (padHandles[i] >= 0) scePadSetVibration(padHandles[i], &stopVibe);
                    }
                    send_vibe_to_daemon(0, 0, (uint8_t)tester.selectedSlot);
                }
            }
        }

        // Tab Switching via L1 / R1 (suspended during Remap Test Mode and Tester Test Mode)
        if (!remapTestMode && !tester.testMode) {
            if (justPressed & ORBIS_PAD_BUTTON_L1) {
                activeTabIndex = (activeTabIndex + numTabs - 1) % numTabs;
                currentTabId = activeTabs[activeTabIndex];
                if (currentTabId != TAB_TESTER && (tester.rumbleFrames > 0 || tester.rumblePhase != 0)) {
                    tester.rumbleFrames = 0;
                    tester.rumblePhase = 0;
                    OrbisPadVibeParam vibe = { 0, 0 };
                    for (int i = 0; i < 4; i++) {
                        if (padHandles[i] >= 0) scePadSetVibration(padHandles[i], &vibe);
                    }
                    send_vibe_to_daemon(0, 0, (uint8_t)tester.selectedSlot);
                }
            }
            if (justPressed & ORBIS_PAD_BUTTON_R1) {
                activeTabIndex = (activeTabIndex + 1) % numTabs;
                currentTabId = activeTabs[activeTabIndex];
                if (currentTabId != TAB_TESTER && (tester.rumbleFrames > 0 || tester.rumblePhase != 0)) {
                    tester.rumbleFrames = 0;
                    tester.rumblePhase = 0;
                    OrbisPadVibeParam vibe = { 0, 0 };
                    for (int i = 0; i < 4; i++) {
                        if (padHandles[i] >= 0) scePadSetVibration(padHandles[i], &vibe);
                    }
                    send_vibe_to_daemon(0, 0, (uint8_t)tester.selectedSlot);
                }
            }
        }

        if (currentTabId != TAB_REMAP_HID) {
            remapTestMode = false;
        }
        if (currentTabId != TAB_TESTER) {
            tester.testMode = false;
            tester.testModeHoldFrames = 0;
        }

        if (currentTabId == TAB_SETTINGS) {
            const int TOTAL_CARDS = 8;

            // Navigate Cards in Settings
            if (justPressed & ORBIS_PAD_BUTTON_UP) {
                selectedCard = (selectedCard + TOTAL_CARDS - 1) % TOTAL_CARDS;
            }
            if (justPressed & ORBIS_PAD_BUTTON_DOWN) {
                selectedCard = (selectedCard + 1) % TOTAL_CARDS;
            }

            // Quick Pro Profile cycle on TRIANGLE
            if (justPressed & ORBIS_PAD_BUTTON_TRIANGLE) {
                int nextPreset = cfg.pro_profile_index + 1;
                if (nextPreset >= (int)ConfigManager::PRO_PROFILE_OPTIONS.size()) nextPreset = 1;
                cfg.apply_pro_profile(nextPreset);
                saveAndApplyActiveTarget();
                toastText = "¡Perfil Maestro Pro activado: " + ConfigManager::PRO_PROFILE_OPTIONS[cfg.pro_profile_index] + "!";
                toastFrames = 180;
            }

            // Value changes (LEFT)
            if (justPressed & ORBIS_PAD_BUTTON_LEFT) {
                bool changed = false;
                if (selectedCard == 0) {
                    if (cfg.pro_profile_index > 0) {
                        cfg.apply_pro_profile(cfg.pro_profile_index - 1);
                        changed = true;
                    }
                } else if (selectedCard == 1) {
                    if (cfg.deadzone_index > 0) { cfg.deadzone_index--; cfg.pro_profile_index = 0; changed = true; }
                } else if (selectedCard == 2) {
                    if (cfg.stick_curve_index > 0) { cfg.stick_curve_index--; cfg.pro_profile_index = 0; changed = true; }
                } else if (selectedCard == 3) {
                    if (cfg.trigger_index > 0) { cfg.trigger_index--; cfg.pro_profile_index = 0; changed = true; }
                } else if (selectedCard == 4) {
                    if (cfg.rapid_fire_index > 0) { cfg.rapid_fire_index--; cfg.pro_profile_index = 0; changed = true; }
                } else if (selectedCard == 5) {
                    cfg.swap_index ^= 1; changed = true;
                } else if (selectedCard == 6) {
                    cfg.rumble_index ^= 1; cfg.pro_profile_index = 0; changed = true;
                } else if (selectedCard == 7) {
                    cfg.switch_index ^= 1; changed = true;
                }
                if (changed) {
                    saveAndApplyActiveTarget();
                }
            }

            // Value changes (RIGHT)
            if (justPressed & ORBIS_PAD_BUTTON_RIGHT) {
                bool changed = false;
                if (selectedCard == 0) {
                    if (cfg.pro_profile_index < (int)ConfigManager::PRO_PROFILE_OPTIONS.size() - 1) {
                        cfg.apply_pro_profile(cfg.pro_profile_index + 1);
                        changed = true;
                    }
                } else if (selectedCard == 1) {
                    if (cfg.deadzone_index < (int)ConfigManager::DEADZONE_OPTIONS.size() - 1) {
                        cfg.deadzone_index++; cfg.pro_profile_index = 0; changed = true;
                    }
                } else if (selectedCard == 2) {
                    if (cfg.stick_curve_index < (int)ConfigManager::STICK_CURVE_OPTIONS.size() - 1) {
                        cfg.stick_curve_index++; cfg.pro_profile_index = 0; changed = true;
                    }
                } else if (selectedCard == 3) {
                    if (cfg.trigger_index < (int)ConfigManager::TRIGGER_OPTIONS.size() - 1) {
                        cfg.trigger_index++; cfg.pro_profile_index = 0; changed = true;
                    }
                } else if (selectedCard == 4) {
                    if (cfg.rapid_fire_index < (int)ConfigManager::RAPID_FIRE_OPTIONS.size() - 1) {
                        cfg.rapid_fire_index++; cfg.pro_profile_index = 0; changed = true;
                    }
                } else if (selectedCard == 5) {
                    cfg.swap_index ^= 1; changed = true;
                } else if (selectedCard == 6) {
                    cfg.rumble_index ^= 1; cfg.pro_profile_index = 0; changed = true;
                } else if (selectedCard == 7) {
                    cfg.switch_index ^= 1; changed = true;
                }
                if (changed) {
                    saveAndApplyActiveTarget();
                }
            }

            // Save on CROSS (X)
            if (justPressed & ORBIS_PAD_BUTTON_CROSS) {
                if (saveAndApplyActiveTarget()) {
                    toastText = activeTargetId.empty()
                        ? "¡Ajustes Globales guardados en /data/anycontroller/anycontroller.ini!"
                        : ("¡Memoria del mando guardada en " + activeTargetId + ".ini!");
                    toastFrames = 180;
                }
            }
        } else if (currentTabId == TAB_TESTER) {
            // --- TESTER & CALIBRATION TAB ---
            if (tester.testMode) {
                bool l1r1Combo = ((curButtons & (ORBIS_PAD_BUTTON_L1 | ORBIS_PAD_BUTTON_R1)) == (ORBIS_PAD_BUTTON_L1 | ORBIS_PAD_BUTTON_R1)) &&
                                 ((justPressed & (ORBIS_PAD_BUTTON_L1 | ORBIS_PAD_BUTTON_R1)) != 0);
                if (curButtons & ORBIS_PAD_BUTTON_OPTIONS) {
                    tester.testModeHoldFrames++;
                } else {
                    tester.testModeHoldFrames = 0;
                }
                if (l1r1Combo || tester.testModeHoldFrames >= 45) {
                    tester.testMode = false;
                    tester.testModeHoldFrames = 0;
                    toastText = "Modo Prueba Libre desactivado";
                    toastFrames = 120;
                }
            } else {
                // Interactive 5-Row Menu (zero overlapping dual-function controls!)
                if (justPressed & ORBIS_PAD_BUTTON_UP) {
                    tester.testerMenuRow = (tester.testerMenuRow + 4) % 5;
                }
                if (justPressed & ORBIS_PAD_BUTTON_DOWN) {
                    tester.testerMenuRow = (tester.testerMenuRow + 1) % 5;
                }
                if (justPressed & ORBIS_PAD_BUTTON_L3) {
                    tester.selectedSlot = (tester.selectedSlot + 1) % 4;
                }

                if (tester.testerMenuRow == 0) {
                    if (justPressed & ORBIS_PAD_BUTTON_LEFT) {
                        tester.selectedSlot = (tester.selectedSlot + 3) % 4;
                    }
                    if (justPressed & (ORBIS_PAD_BUTTON_RIGHT | ORBIS_PAD_BUTTON_CROSS)) {
                        tester.selectedSlot = (tester.selectedSlot + 1) % 4;
                    }
                } else if (tester.testerMenuRow == 1) {
                    if (justPressed & ORBIS_PAD_BUTTON_LEFT) {
                        tester.testerModelOverride = (tester.testerModelOverride + 2) % 3;
                    }
                    if (justPressed & (ORBIS_PAD_BUTTON_RIGHT | ORBIS_PAD_BUTTON_CROSS)) {
                        tester.testerModelOverride = (tester.testerModelOverride + 1) % 3;
                    }
                } else if (tester.testerMenuRow == 2) {
                    if (justPressed & ORBIS_PAD_BUTTON_CROSS) {
                        tester.testMode = true;
                        tester.testModeHoldFrames = 0;
                    }
                } else if (tester.testerMenuRow == 3) {
                    if (justPressed & ORBIS_PAD_BUTTON_CROSS) {
                        tester.rumbleFrames = 180; // 3 seconds isolated in tester.selectedSlot
                    }
                } else if (tester.testerMenuRow == 4) {
                    if (justPressed & ORBIS_PAD_BUTTON_CROSS) {
                        if (tester.calib.completed) {
                            cfg.deadzone_index = tester.calib.suggestedIndex;
                            if (saveAndApplyActiveTarget()) {
                                toastText = "¡Zona muerta auto-calibrada y guardada!";
                                toastFrames = 180;
                            }
                            tester.calib.completed = false;
                        } else if (!tester.calib.active) {
                            tester.calib.active = true;
                            tester.calib.completed = false;
                            tester.calib.currentFrame = 0;
                            tester.calib.maxNoise = 0;
                        }
                    }
                }
            }

            // Real-time processing for Isolated Slot Rumble Test
            if (tester.rumbleFrames > 0) {
                tester.rumbleFrames--;
                OrbisPadVibeParam vibe;
                if (tester.rumbleFrames > 120) {
                    tester.rumblePhase = 1;
                    vibe.lgMotor = 255;
                    vibe.smMotor = 0;
                } else if (tester.rumbleFrames > 60) {
                    tester.rumblePhase = 2;
                    vibe.lgMotor = 0;
                    vibe.smMotor = 255;
                } else {
                    tester.rumblePhase = 3;
                    vibe.lgMotor = 255;
                    vibe.smMotor = 255;
                }
                OrbisPadVibeParam zeroVibe = { 0, 0 };
                for (int i = 0; i < 4; i++) {
                    if (padHandles[i] >= 0) {
                        scePadSetVibration(padHandles[i], (i == tester.selectedSlot) ? &vibe : &zeroVibe);
                    }
                }
                send_vibe_to_daemon(vibe.lgMotor, vibe.smMotor, (uint8_t)tester.selectedSlot);
            } else if (tester.rumblePhase != 0) {
                tester.rumblePhase = 0;
                OrbisPadVibeParam vibe = { 0, 0 };
                for (int i = 0; i < 4; i++) {
                    if (padHandles[i] >= 0) scePadSetVibration(padHandles[i], &vibe);
                }
                send_vibe_to_daemon(0, 0, (uint8_t)tester.selectedSlot);
            }

            // Real-time processing for Drift Calibration (uses raw padData before live filter)
            if (tester.calib.active) {
                int devLX = abs((int)padData.leftStick.x - 128);
                int devLY = abs((int)padData.leftStick.y - 128);
                int devRX = abs((int)padData.rightStick.x - 128);
                int devRY = abs((int)padData.rightStick.y - 128);

                if (devLX > tester.calib.maxNoise) tester.calib.maxNoise = devLX;
                if (devLY > tester.calib.maxNoise) tester.calib.maxNoise = devLY;
                if (devRX > tester.calib.maxNoise) tester.calib.maxNoise = devRX;
                if (devRY > tester.calib.maxNoise) tester.calib.maxNoise = devRY;

                tester.calib.currentFrame++;
                if (tester.calib.currentFrame >= tester.calib.totalFrames) {
                    tester.calib.active = false;
                    tester.calib.completed = true;

                    int needed = tester.calib.maxNoise + 2;
                    int bestIdx = (int)ConfigManager::DEADZONE_VALUES.size() - 1;
                    for (size_t i = 0; i < ConfigManager::DEADZONE_VALUES.size(); i++) {
                        if (ConfigManager::DEADZONE_VALUES[i] >= needed) {
                            bestIdx = (int)i;
                            break;
                        }
                    }
                    tester.calib.suggestedIndex = bestIdx;
                }
            } else if (!btClaimedSlots[tester.selectedSlot]) {
                // Apply configured deadzone, trigger threshold, and L1/L2 swap live in Tester when previewing DS4 slot
                int dz = ConfigManager::DEADZONE_VALUES[cfg.deadzone_index];
                if (dz > 0) {
                    int lx = (int)padData.leftStick.x - 128;
                    int ly = (int)padData.leftStick.y - 128;
                    if (lx * lx + ly * ly <= dz * dz) {
                        padData.leftStick.x = 128;
                        padData.leftStick.y = 128;
                    }
                    int rx = (int)padData.rightStick.x - 128;
                    int ry = (int)padData.rightStick.y - 128;
                    if (rx * rx + ry * ry <= dz * dz) {
                        padData.rightStick.x = 128;
                        padData.rightStick.y = 128;
                    }
                }
                int th = ConfigManager::TRIGGER_VALUES[cfg.trigger_index];
                if (padData.analogButtons.l2 >= th) padData.buttons |= ORBIS_PAD_BUTTON_L2;
                else padData.buttons &= ~ORBIS_PAD_BUTTON_L2;
                if (padData.analogButtons.r2 >= th) padData.buttons |= ORBIS_PAD_BUTTON_R2;
                else padData.buttons &= ~ORBIS_PAD_BUTTON_R2;

                if (cfg.swap_index != 0) {
                    bool l1 = (padData.buttons & ORBIS_PAD_BUTTON_L1) != 0;
                    bool r1 = (padData.buttons & ORBIS_PAD_BUTTON_R1) != 0;
                    bool l2 = (padData.buttons & ORBIS_PAD_BUTTON_L2) != 0;
                    bool r2 = (padData.buttons & ORBIS_PAD_BUTTON_R2) != 0;
                    padData.buttons &= ~(ORBIS_PAD_BUTTON_L1 | ORBIS_PAD_BUTTON_R1 | ORBIS_PAD_BUTTON_L2 | ORBIS_PAD_BUTTON_R2);
                    if (l1) padData.buttons |= ORBIS_PAD_BUTTON_L2;
                    if (l2) padData.buttons |= ORBIS_PAD_BUTTON_L1;
                    if (r1) padData.buttons |= ORBIS_PAD_BUTTON_R2;
                    if (r2) padData.buttons |= ORBIS_PAD_BUTTON_R1;
                    padData.analogButtons.l2 = l1 ? 255 : 0;
                    padData.analogButtons.r2 = r1 ? 255 : 0;
                }
            }
        } else if (currentTabId == TAB_DUALSENSE && hasDualSense) {
            // --- DUALSENSE MANAGEMENT TAB ---
            const int TOTAL_DS_CARDS = 8;
            if (justPressed & ORBIS_PAD_BUTTON_UP) {
                dsSelectedCard = (dsSelectedCard + TOTAL_DS_CARDS - 1) % TOTAL_DS_CARDS;
            }
            if (justPressed & ORBIS_PAD_BUTTON_DOWN) {
                dsSelectedCard = (dsSelectedCard + 1) % TOTAL_DS_CARDS;
            }

            // Value changes (LEFT)
            if (justPressed & ORBIS_PAD_BUTTON_LEFT) {
                bool dsChanged = false;
                if (dsSelectedCard == 0) {
                    if (cfg.ds_lightbar_mode > 0) { cfg.ds_lightbar_mode--; dsChanged = true; }
                } else if (dsSelectedCard == 1) {
                    if (cfg.ds_lightbar_color > 0) {
                        cfg.ds_lightbar_color--;
                        if (cfg.ds_lightbar_mode == 0 || cfg.ds_lightbar_mode == 3) cfg.ds_lightbar_mode = 1;
                        dsChanged = true;
                    }
                } else if (dsSelectedCard == 2) {
                    if (cfg.ds_brightness > 0) { cfg.ds_brightness--; dsChanged = true; }
                } else if (dsSelectedCard == 3) {
                    if (cfg.ds_trigger_mode > 0) { cfg.ds_trigger_mode--; dsChanged = true; }
                } else if (dsSelectedCard == 4) {
                    if (cfg.ds_player_leds > 0) { cfg.ds_player_leds--; dsChanged = true; }
                } else if (dsSelectedCard == 5) {
                    if (cfg.ds_speaker_volume >= 5) {
                        cfg.ds_speaker_volume -= 5;
                    } else {
                        cfg.ds_speaker_volume = 0;
                    }
                    dsChanged = true;
                } else if (dsSelectedCard == 6) {
                    cfg.ds_mute_action ^= 1; dsChanged = true;
                } else if (dsSelectedCard == 7) {
                    cfg.ds_bt_mode ^= 1; dsChanged = true;
                }
                if (dsChanged) {
                    saveAndApplyActiveTarget();
                }
            }

            // Value changes (RIGHT)
            if (justPressed & ORBIS_PAD_BUTTON_RIGHT) {
                bool dsChanged = false;
                if (dsSelectedCard == 0) {
                    if (cfg.ds_lightbar_mode < (int)ConfigManager::DS_LIGHTBAR_OPTIONS.size() - 1) {
                        cfg.ds_lightbar_mode++; dsChanged = true;
                    }
                } else if (dsSelectedCard == 1) {
                    if (cfg.ds_lightbar_color < (int)ConfigManager::DS_COLOR_OPTIONS.size() - 1) {
                        cfg.ds_lightbar_color++;
                        if (cfg.ds_lightbar_mode == 0 || cfg.ds_lightbar_mode == 3) cfg.ds_lightbar_mode = 1;
                        dsChanged = true;
                    }
                } else if (dsSelectedCard == 2) {
                    if (cfg.ds_brightness < (int)ConfigManager::DS_BRIGHTNESS_OPTIONS.size() - 1) {
                        cfg.ds_brightness++; dsChanged = true;
                    }
                } else if (dsSelectedCard == 3) {
                    if (cfg.ds_trigger_mode < (int)ConfigManager::DS_TRIGGER_OPTIONS.size() - 1) {
                        cfg.ds_trigger_mode++; dsChanged = true;
                    }
                } else if (dsSelectedCard == 4) {
                    if (cfg.ds_player_leds < (int)ConfigManager::DS_PLAYER_LEDS_OPTIONS.size() - 1) {
                        cfg.ds_player_leds++; dsChanged = true;
                    }
                } else if (dsSelectedCard == 5) {
                    if (cfg.ds_speaker_volume <= 95) {
                        cfg.ds_speaker_volume += 5;
                    } else {
                        cfg.ds_speaker_volume = 100;
                    }
                    dsChanged = true;
                } else if (dsSelectedCard == 6) {
                    cfg.ds_mute_action ^= 1; dsChanged = true;
                } else if (dsSelectedCard == 7) {
                    cfg.ds_bt_mode ^= 1; dsChanged = true;
                }
                if (dsChanged) {
                    saveAndApplyActiveTarget();
                }
            }

            // Toggle Mute on SQUARE when on Speaker Volume card
            if ((justPressed & ORBIS_PAD_BUTTON_SQUARE) && dsSelectedCard == 5) {
                if (cfg.ds_speaker_volume > 0) {
                    cfg.ds_speaker_volume = 0;
                } else {
                    cfg.ds_speaker_volume = 80;
                }
                saveAndApplyActiveTarget();
            }

            // Open PS4 User Profile Selector ("¿Quién está usando este mando?") on TRIANGLE
            if (justPressed & ORBIS_PAD_BUTTON_TRIANGLE) {
                request_daemon_rebind_profile();
                toastText = "¡Abriendo selector de perfil PS4 ('¿Quién está usando este mando?')!";
                toastFrames = 180;
            }

            // Save on CROSS (X)
            if (justPressed & ORBIS_PAD_BUTTON_CROSS) {
                if (saveAndApplyActiveTarget()) {
                    toastText = activeTargetId.empty()
                        ? "¡Configuración Global DualSense PS5 guardada!"
                        : ("¡Memoria PS5 guardada en " + activeTargetId + ".ini!");
                    toastFrames = 180;
                }
            }
        } else if (currentTabId == TAB_DUALSHOCK4) {
            // --- NATIVE PS4 DUALSHOCK 4 CONFIGURATION TAB ---
            const int TOTAL_DS4_CARDS = 6;
            if (justPressed & ORBIS_PAD_BUTTON_UP) {
                ds4SelectedCard = (ds4SelectedCard + TOTAL_DS4_CARDS - 1) % TOTAL_DS4_CARDS;
            }
            if (justPressed & ORBIS_PAD_BUTTON_DOWN) {
                ds4SelectedCard = (ds4SelectedCard + 1) % TOTAL_DS4_CARDS;
            }

            if (justPressed & ORBIS_PAD_BUTTON_LEFT) {
                bool ds4Changed = false;
                if (ds4SelectedCard == 0) {
                    if (cfg.ds4_lightbar_mode > 0) { cfg.ds4_lightbar_mode--; ds4Changed = true; }
                } else if (ds4SelectedCard == 1) {
                    if (cfg.ds4_lightbar_color > 0) {
                        cfg.ds4_lightbar_color--;
                        if (cfg.ds4_lightbar_mode == 0 || cfg.ds4_lightbar_mode == 3) cfg.ds4_lightbar_mode = 1;
                        ds4Changed = true;
                    }
                } else if (ds4SelectedCard == 2) {
                    if (cfg.ds4_brightness > 0) { cfg.ds4_brightness--; ds4Changed = true; }
                } else if (ds4SelectedCard == 3) {
                    if (cfg.ds4_rumble_boost > 0) { cfg.ds4_rumble_boost--; ds4Changed = true; }
                } else if (ds4SelectedCard == 4) {
                    cfg.ds4_touchpad_mode ^= 1; ds4Changed = true;
                } else if (ds4SelectedCard == 5) {
                    if (cfg.ds4_gyro_deadband > 0) { cfg.ds4_gyro_deadband--; ds4Changed = true; }
                }
                if (ds4Changed) {
                    saveAndApplyActiveTarget();
                }
            }

            if (justPressed & ORBIS_PAD_BUTTON_RIGHT) {
                bool ds4Changed = false;
                if (ds4SelectedCard == 0) {
                    if (cfg.ds4_lightbar_mode < (int)ConfigManager::DS4_LIGHTBAR_OPTIONS.size() - 1) {
                        cfg.ds4_lightbar_mode++; ds4Changed = true;
                    }
                } else if (ds4SelectedCard == 1) {
                    if (cfg.ds4_lightbar_color < (int)ConfigManager::DS4_COLOR_OPTIONS.size() - 1) {
                        cfg.ds4_lightbar_color++;
                        if (cfg.ds4_lightbar_mode == 0 || cfg.ds4_lightbar_mode == 3) cfg.ds4_lightbar_mode = 1;
                        ds4Changed = true;
                    }
                } else if (ds4SelectedCard == 2) {
                    if (cfg.ds4_brightness < (int)ConfigManager::DS4_BRIGHTNESS_OPTIONS.size() - 1) {
                        cfg.ds4_brightness++; ds4Changed = true;
                    }
                } else if (ds4SelectedCard == 3) {
                    if (cfg.ds4_rumble_boost < (int)ConfigManager::DS4_RUMBLE_BOOST_OPTIONS.size() - 1) {
                        cfg.ds4_rumble_boost++; ds4Changed = true;
                    }
                } else if (ds4SelectedCard == 4) {
                    cfg.ds4_touchpad_mode ^= 1; ds4Changed = true;
                } else if (ds4SelectedCard == 5) {
                    if (cfg.ds4_gyro_deadband < (int)ConfigManager::DS4_GYRO_OPTIONS.size() - 1) {
                        cfg.ds4_gyro_deadband++; ds4Changed = true;
                    }
                }
                if (ds4Changed) {
                    saveAndApplyActiveTarget();
                }
            }

            // Quick Isolated Rumble Test on SQUARE in DualShock 4 tab
            int targetRumbleSlot = (targets[activeTargetIndex].slotIndex >= 0) ? targets[activeTargetIndex].slotIndex : tester.selectedSlot;
            if (targetRumbleSlot < 0 || targetRumbleSlot > 3) targetRumbleSlot = 0;

            if (justPressed & ORBIS_PAD_BUTTON_SQUARE) {
                uint8_t intensity = 200;
                if (cfg.ds4_rumble_boost == 1) intensity = 120;
                else if (cfg.ds4_rumble_boost == 2) intensity = 255;
                else if (cfg.ds4_rumble_boost == 3) intensity = 0;

                OrbisPadVibeParam vibe = { intensity, intensity };
                OrbisPadVibeParam zeroVibe = { 0, 0 };
                for (int i = 0; i < 4; i++) {
                    if (padHandles[i] >= 0) {
                        scePadSetVibration(padHandles[i], (i == targetRumbleSlot) ? &vibe : &zeroVibe);
                    }
                }
                if (btClaimedSlots[targetRumbleSlot] || isDualSenseActive) {
                    send_vibe_to_daemon(vibe.lgMotor, vibe.smMotor, (uint8_t)targetRumbleSlot);
                }
                tester.rumblePhase = 3;
                tester.rumbleFrames = 60; // 1 second pulse
                char vibeMsg[96];
                snprintf(vibeMsg, sizeof(vibeMsg), "Probando vibración SOLO en Jugador %d...", targetRumbleSlot + 1);
                toastText = vibeMsg;
                toastFrames = 90;
            }

            if (tester.rumbleFrames > 0) {
                tester.rumbleFrames--;
                if (tester.rumbleFrames == 0) {
                    tester.rumblePhase = 0;
                    OrbisPadVibeParam stopVibe = { 0, 0 };
                    for (int i = 0; i < 4; i++) {
                        if (padHandles[i] >= 0) scePadSetVibration(padHandles[i], &stopVibe);
                    }
                    send_vibe_to_daemon(0, 0, (uint8_t)targetRumbleSlot);
                }
            }

            // Save on CROSS (X)
            if (justPressed & ORBIS_PAD_BUTTON_CROSS) {
                if (saveAndApplyActiveTarget()) {
                    toastText = activeTargetId.empty()
                        ? "¡Ajustes Globales DualShock 4 aplicados en vivo!"
                        : ("¡Memoria DualShock 4 guardada en " + activeTargetId + ".ini!");
                    toastFrames = 180;
                }
            }
        } else if (currentTabId == TAB_REMAP_HID && hasHid) {
            // --- REMAP HID TAB ---
            // R3 toggles between Edit Mode and Test Mode
            if (justPressed & ORBIS_PAD_BUTTON_R3) {
                remapTestMode = !remapTestMode;
            }

            if (!remapTestMode) {
                if (justPressed & ORBIS_PAD_BUTTON_UP) {
                    remapSelectedRow = (remapSelectedRow + ACT_COUNT - 1) % ACT_COUNT;
                }
                if (justPressed & ORBIS_PAD_BUTTON_DOWN) {
                    remapSelectedRow = (remapSelectedRow + 1) % ACT_COUNT;
                }
                if (justPressed & ORBIS_PAD_BUTTON_LEFT) {
                    hidProfile.cycle_button(remapSelectedRow, -1);
                }
                if (justPressed & ORBIS_PAD_BUTTON_RIGHT) {
                    hidProfile.cycle_button(remapSelectedRow, 1);
                }
                if (justPressed & ORBIS_PAD_BUTTON_CROSS) {
                    if (hidProfile.save(hidStatus.vid, hidStatus.pid, hidStatus.name)) {
                        toastText = "¡Perfil guardado y activo!";
                        toastFrames = 180;
                    }
                }
                if (justPressed & ORBIS_PAD_BUTTON_SQUARE) {
                    hidProfile.reset_defaults(hidStatus.nbuttons);
                    toastText = "Valores predeterminados restaurados (pulsa X para guardar)";
                    toastFrames = 180;
                }
                if (justPressed & ORBIS_PAD_BUTTON_TRIANGLE) {
                    hidProfile.load(hidStatus.vid, hidStatus.pid, hidStatus.nbuttons);
                    toastText = "Perfil recargado desde el disco";
                    toastFrames = 180;
                }
            }
        } else {
            // In Info tab: CROSS only triggers PKG download / check (single unambiguous role)
            if (justPressed & ORBIS_PAD_BUTTON_CROSS) {
                if (updater.get_state() == UPDATER_UPDATE_AVAILABLE) {
                    updater.start_download();
                    toastText = "Descarga iniciada en /data/pkg/ (lista para GoldHEN Package Installer)";
                    toastFrames = 180;
                } else if (updater.get_state() != UPDATER_CHECKING && updater.get_state() != UPDATER_DOWNLOADING) {
                    updater.start_check();
                    toastText = "Buscando actualizaciones en GitHub Releases...";
                    toastFrames = 150;
                }
            }

            // In Info tab: SQUARE re-runs installation & ensures files/autoboot are up-to-date
            if (justPressed & ORBIS_PAD_BUTTON_SQUARE) {
                Installer::runInstallation();
                notify_daemon_reload();
                toastText = "Payload ANYCONTROLLER reinstalado en /data/payloads/anycontroller.bin";
                toastFrames = 180;
            }
        }

        // Auto-notify user as soon as an online update is detected
        UpdaterState curUpState = updater.get_state();
        if (lastUpdaterState != UPDATER_UPDATE_AVAILABLE && curUpState == UPDATER_UPDATE_AVAILABLE) {
            toastText = "¡Actualización disponible: " + updater.get_latest_version() + "! Pulsa L1/R1 para ver Sistema";
            toastFrames = 240; // ~4s notification toast
        }
        lastUpdaterState = curUpState;

        if (toastFrames > 0) {
            toastFrames--;
        }

        // Render UI
        ui->render(scene, cfg, currentTabId, selectedCard, toastFrames, padData, tester, toastText.c_str(),
                   hasHid, &hidStatus, &hidProfile, remapSelectedRow, remapTestMode, &updater,
                   isDualSenseActive, dsSelectedCard, ds4SelectedCard, !isDualSenseActive,
                   &targets, activeTargetIndex);

        // Flip & VSync
        scene->SubmitFlip(frameID);
        scene->FrameWait(frameID);
        scene->FrameBufferSwap();
        frameID++;
    }

    // Persist active profile & flush daemon reload before releasing scePad handles
    configMgr.saveTarget(activeTargetId);
    notify_daemon_reload();
    usleep(250000);

    for (int i = 0; i < 4; i++) {
        if (padHandles[i] >= 0) {
            OrbisPadVibeParam vibe = { 0, 0 };
            scePadSetVibration(padHandles[i], &vibe);
            scePadClose(padHandles[i]);
        }
    }
    send_vibe_to_daemon(0, 0, (uint8_t)tester.selectedSlot);

    // Gracefully exit back to PS4 shell without CE-34878-0
    sceSystemServiceLoadExec("exit", NULL);
    exit(0);

    return 0;
}
