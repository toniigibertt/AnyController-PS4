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

static void send_vibe_to_daemon(uint8_t lgMotor, uint8_t smMotor) {
    static uint8_t s_last_sent_lg = 0;
    static uint8_t s_last_sent_sm = 0;
    static bool s_has_sent = false;

    if (s_has_sent && lgMotor == s_last_sent_lg && smMotor == s_last_sent_sm) {
        return; // State unchanged, avoid spamming filesystem I/O on 60fps render loop
    }
    s_last_sent_lg = lgMotor;
    s_last_sent_sm = smMotor;
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
        uint8_t data[2] = { lgMotor, smMotor };
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

static void apply_live_pad_lightbar(int padHandles[4], const AppConfig& cfg) {
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
    int br = cfg.ds4_brightness;
    if (br < 0 || br > 2) br = 0;
    int hw_base_br = (cfg.ds4_lightbar_mode == 3) ? 2 : br;

    OrbisUserServiceLoginUserIdList userList;
    memset(&userList, 0, sizeof(userList));
    bool hasUsers = (sceUserServiceGetLoginUserIdList(&userList) == 0);

    for (int i = 0; i < 4; i++) {
        if (padHandles[i] < 0) continue;

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
                // Fallback for firmwares rejecting {0,0,0}: BaseBrightness=2 truncates (1*15)/255 = 0
                OrbisPadColor dim_off = { 1, 0, 0, 0 };
                scePadSetLightBar(padHandles[i], &dim_off);
            }
            continue;
        } else if (cfg.ds4_lightbar_mode == 0) {
            // Mode 0 with Medium/Low brightness override
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
    apply_live_pad_lightbar(padHandles, configMgr.getConfig());

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
            apply_live_pad_lightbar(padHandles, configMgr.getConfig());
        } else if (frameID % 3 == 0 &&
                   (configMgr.getConfig().ds4_lightbar_mode == 4 || configMgr.getConfig().ds4_lightbar_mode == 5)) {
            apply_live_pad_lightbar(padHandles, configMgr.getConfig());
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

        // Smart Active Slot Auto-Detection:
        // Automatically switch tester.selectedSlot to whichever controller slot is actively sending inputs
        // (suspended when in Input Tester Test Mode to prevent accidental slot jumping)
        if (!tester.testMode) {
            for (int i = 0; i < 4; i++) {
                if (tester.slotConnected[i]) {
                    const OrbisPadData& d = padDataSlots[i];
                    bool hasStick = (abs((int)d.leftStick.x - 128) > 20 ||
                                     abs((int)d.leftStick.y - 128) > 20 ||
                                     abs((int)d.rightStick.x - 128) > 20 ||
                                     abs((int)d.rightStick.y - 128) > 20);
                    bool hasTrig = (d.analogButtons.l2 > 30 || d.analogButtons.r2 > 30);
                    bool hasBtn = (d.buttons != 0);
                    if (hasStick || hasTrig || hasBtn) {
                        tester.selectedSlot = i;
                        break;
                    }
                }
            }
        }
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

        // Poll HID / DualSense controller status every 15 frames (4 Hz) for live battery & charging updates
        if (frameID % 15 == 0) {
            HidProfile::read_status(hidStatus);
        }
        bool isHidType = (hidStatus.type == "generic_hid" ||
                          hidStatus.type == "Generic HID Gamepad" ||
                          hidStatus.type.find("hid") != std::string::npos ||
                          hidStatus.type.find("HID") != std::string::npos);
        bool hasHid = (hidStatus.connected && isHidType);

        // Context-aware controller tabs:
        // Show DualSense PS5 tab ONLY when a PS5 DualSense controller is connected.
        // Show DualShock 4 tab ONLY when using a native PS4 DualShock 4 controller (and DualSense is not active).
        bool isDualSenseActive = (hidStatus.connected &&
                                  (hidStatus.type == "dualsense" ||
                                   hidStatus.type == "dualsense_bt" ||
                                   (hidStatus.vid == 0x054C && (hidStatus.pid == 0x0CE6 || hidStatus.pid == 0x0DF2))));
        bool hasDualSense = isDualSenseActive;
        bool hasDualShock4 = !isDualSenseActive;

        if (hasHid && !hidLoaded) {
            hidProfile.load(hidStatus.vid, hidStatus.pid, hidStatus.nbuttons);
            hidLoaded = true;
        } else if (!hasHid) {
            hidLoaded = false;
        }

        // Dynamically build active tab list based on the connected controller
        std::vector<int> activeTabs;
        activeTabs.push_back(TAB_SETTINGS);
        if (hasDualSense) {
            activeTabs.push_back(TAB_DUALSENSE);
        }
        if (hasDualShock4) {
            activeTabs.push_back(TAB_DUALSHOCK4);
        }
        activeTabs.push_back(TAB_TESTER);
        if (hasHid) {
            activeTabs.push_back(TAB_REMAP_HID);
        }
        activeTabs.push_back(TAB_INFO);

        // If the active controller changed while viewing the controller-specific tab, switch seamlessly
        if (currentTabId == TAB_DUALSENSE && !hasDualSense) {
            currentTabId = TAB_DUALSHOCK4;
        } else if (currentTabId == TAB_DUALSHOCK4 && !hasDualShock4) {
            currentTabId = TAB_DUALSENSE;
        } else if (currentTabId == TAB_REMAP_HID && !hasHid) {
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

        // Exit on OPTIONS or CIRCLE (suspended during Remap Test Mode and Tester Test Mode)
        if (!remapTestMode && !tester.testMode && ((justPressed & ORBIS_PAD_BUTTON_OPTIONS) || (justPressed & ORBIS_PAD_BUTTON_CIRCLE))) {
            break;
        }

        // Tab Switching via L1 / R1 (suspended during Remap Test Mode and Tester Test Mode)
        if (!remapTestMode && !tester.testMode) {
            if (justPressed & ORBIS_PAD_BUTTON_L1) {
                activeTabIndex = (activeTabIndex + numTabs - 1) % numTabs;
                currentTabId = activeTabs[activeTabIndex];
                if (currentTabId != TAB_TESTER && tester.rumblePhase != 0) {
                    tester.rumbleFrames = 0;
                    tester.rumblePhase = 0;
                    OrbisPadVibeParam vibe = { 0, 0 };
                    for (int i = 0; i < 4; i++) {
                        if (padHandles[i] >= 0) scePadSetVibration(padHandles[i], &vibe);
                    }
                    send_vibe_to_daemon(0, 0);
                }
            }
            if (justPressed & ORBIS_PAD_BUTTON_R1) {
                activeTabIndex = (activeTabIndex + 1) % numTabs;
                currentTabId = activeTabs[activeTabIndex];
                if (currentTabId != TAB_TESTER && tester.rumblePhase != 0) {
                    tester.rumbleFrames = 0;
                    tester.rumblePhase = 0;
                    OrbisPadVibeParam vibe = { 0, 0 };
                    for (int i = 0; i < 4; i++) {
                        if (padHandles[i] >= 0) scePadSetVibration(padHandles[i], &vibe);
                    }
                    send_vibe_to_daemon(0, 0);
                }
            }
        }

        if (currentTabId != TAB_REMAP_HID) {
            remapTestMode = false;
        }
        if (currentTabId != TAB_TESTER) {
            tester.testMode = false;
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
                configMgr.save();
                notify_daemon_reload();
                apply_live_pad_lightbar(padHandles, cfg);
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
                    configMgr.save();
                    notify_daemon_reload();
                    apply_live_pad_lightbar(padHandles, cfg);
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
                    configMgr.save();
                    notify_daemon_reload();
                    apply_live_pad_lightbar(padHandles, cfg);
                }
            }

            // Save on CROSS (X)
            if (justPressed & ORBIS_PAD_BUTTON_CROSS) {
                if (configMgr.save()) {
                    notify_daemon_reload();
                    apply_live_pad_lightbar(padHandles, cfg);
                    toastText = "¡Ajustes Pro guardados en /data/anycontroller/anycontroller.ini!";
                    toastFrames = 180; // ~3 seconds toast feedback
                }
            }
        } else if (currentTabId == TAB_TESTER) {
            // --- TESTER & CALIBRATION TAB ---
            // R3 toggles between Normal Mode and isolated Test Mode
            if (justPressed & ORBIS_PAD_BUTTON_R3) {
                tester.testMode = !tester.testMode;
            }

            if (!tester.testMode) {
                // L3 or SHARE: Cycle between connected controller slots (never jump to disconnected slot on D-Pad/L2/R2!)
                if (justPressed & ORBIS_PAD_BUTTON_L3) {
                    for (int step = 1; step <= 4; step++) {
                        int cand = (tester.selectedSlot + step) % 4;
                        if (tester.slotConnected[cand]) {
                            tester.selectedSlot = cand;
                            break;
                        }
                    }
                }

                // Square: Start Vibration Test
                if (justPressed & ORBIS_PAD_BUTTON_SQUARE) {
                    tester.rumbleFrames = 180; // 3 seconds
                }

                // Triangle: Start Drift Auto-Calibration
                if (justPressed & ORBIS_PAD_BUTTON_TRIANGLE) {
                    tester.calib.active = true;
                    tester.calib.completed = false;
                    tester.calib.currentFrame = 0;
                    tester.calib.maxNoise = 0;
                }

                // Cross: Apply suggested deadzone if calibration completed
                if (justPressed & ORBIS_PAD_BUTTON_CROSS) {
                    if (tester.calib.completed) {
                        cfg.deadzone_index = tester.calib.suggestedIndex;
                        if (configMgr.save()) {
                            notify_daemon_reload();
                            toastText = "¡Zona muerta auto-calibrada y guardada!";
                            toastFrames = 180;
                        }
                        tester.calib.completed = false;
                    }
                }
            }

            // Real-time processing for Rumble Test
            if (tester.rumbleFrames > 0) {
                tester.rumbleFrames--;
                OrbisPadVibeParam vibe;
                if (tester.rumbleFrames > 120) {
                    // Phase 1: Left heavy motor
                    tester.rumblePhase = 1;
                    vibe.lgMotor = 255;
                    vibe.smMotor = 0;
                } else if (tester.rumbleFrames > 60) {
                    // Phase 2: Right light motor
                    tester.rumblePhase = 2;
                    vibe.lgMotor = 0;
                    vibe.smMotor = 255;
                } else {
                    // Phase 3: Both motors
                    tester.rumblePhase = 3;
                    vibe.lgMotor = 255;
                    vibe.smMotor = 255;
                }
                for (int i = 0; i < 4; i++) {
                    if (padHandles[i] >= 0) scePadSetVibration(padHandles[i], &vibe);
                }
                send_vibe_to_daemon(vibe.lgMotor, vibe.smMotor);
            } else if (tester.rumblePhase != 0) {
                tester.rumblePhase = 0;
                OrbisPadVibeParam vibe = { 0, 0 };
                for (int i = 0; i < 4; i++) {
                    if (padHandles[i] >= 0) scePadSetVibration(padHandles[i], &vibe);
                }
                send_vibe_to_daemon(0, 0);
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

                    // Find smallest deadzone >= (maxNoise + 2) in DEADZONE_VALUES
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
            } else if (!hidStatus.connected) {
                // Apply configured deadzone, trigger threshold, and L1/L2 swap live in Tester when using standard pad
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
                    configMgr.save();
                    notify_daemon_reload();
                    apply_live_pad_lightbar(padHandles, cfg);
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
                    configMgr.save();
                    notify_daemon_reload();
                    apply_live_pad_lightbar(padHandles, cfg);
                }
            }

            // Toggle Mute on SQUARE when on Speaker Volume card
            if ((justPressed & ORBIS_PAD_BUTTON_SQUARE) && dsSelectedCard == 5) {
                if (cfg.ds_speaker_volume > 0) {
                    cfg.ds_speaker_volume = 0;
                } else {
                    cfg.ds_speaker_volume = 80;
                }
                configMgr.save();
                notify_daemon_reload();
            }

            // Open PS4 User Profile Selector ("¿Quién está usando este mando?") on TRIANGLE
            if (justPressed & ORBIS_PAD_BUTTON_TRIANGLE) {
                request_daemon_rebind_profile();
                toastText = "¡Abriendo selector de perfil PS4 ('¿Quién está usando este mando?')!";
                toastFrames = 180;
            }

            // Save on CROSS (X)
            if (justPressed & ORBIS_PAD_BUTTON_CROSS) {
                if (configMgr.save()) {
                    notify_daemon_reload();
                    toastText = "¡Configuración exclusiva de PS5 DualSense guardada!";
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
                    configMgr.save();
                    notify_daemon_reload();
                    apply_live_pad_lightbar(padHandles, cfg);
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
                    configMgr.save();
                    notify_daemon_reload();
                    apply_live_pad_lightbar(padHandles, cfg);
                }
            }

            // Quick Rumble Test on SQUARE in DualShock 4 tab
            if (justPressed & ORBIS_PAD_BUTTON_SQUARE) {
                uint8_t intensity = 200;
                if (cfg.ds4_rumble_boost == 1) intensity = 120;
                else if (cfg.ds4_rumble_boost == 2) intensity = 255;
                else if (cfg.ds4_rumble_boost == 3) intensity = 0;

                OrbisPadVibeParam vibe = { intensity, intensity };
                for (int i = 0; i < 4; i++) {
                    if (padHandles[i] >= 0) scePadSetVibration(padHandles[i], &vibe);
                }
                tester.rumblePhase = 3;
                tester.rumbleFrames = 60; // 1 second pulse
                toastText = "Probando motores de vibración del DualShock 4...";
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
                }
            }

            // Save on CROSS (X)
            if (justPressed & ORBIS_PAD_BUTTON_CROSS) {
                if (configMgr.save()) {
                    notify_daemon_reload();
                    apply_live_pad_lightbar(padHandles, cfg);
                    toastText = "¡Ajustes del mando DualShock 4 aplicados en vivo!";
                    toastFrames = 180;
                }
            }
        } else if (currentTabId == TAB_REMAP_HID && hasHid) {
            // --- REMAP HID TAB ---
            // R3 toggles between Edit Mode and Test Mode (no intrusive toast needed)
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
            // In Info tab: TRIANGLE triggers PKG download if update is available
            if ((justPressed & ORBIS_PAD_BUTTON_TRIANGLE) && updater.get_state() == UPDATER_UPDATE_AVAILABLE) {
                updater.start_download();
                toastText = "Descarga iniciada: anycontroller_companion.pkg -> /data/pkg/";
                toastFrames = 180;
            }

            // In Info tab: SQUARE re-runs installation & ensures files/autoboot are up-to-date
            if (justPressed & ORBIS_PAD_BUTTON_SQUARE) {
                Installer::runInstallation();
                notify_daemon_reload();
                toastText = "AnyController reinstalado en /data/payloads/anycontroller.bin";
                toastFrames = 180;
            }

            // In Info tab: CROSS or LEFT returns to Settings
            if ((justPressed & ORBIS_PAD_BUTTON_CROSS) || (justPressed & ORBIS_PAD_BUTTON_LEFT)) {
                currentTabId = TAB_SETTINGS;
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
                   hasDualSense, dsSelectedCard, ds4SelectedCard, hasDualShock4);

        // Flip & VSync
        scene->SubmitFlip(frameID);
        scene->FrameWait(frameID);
        scene->FrameBufferSwap();
        frameID++;
    }

    for (int i = 0; i < 4; i++) {
        if (padHandles[i] >= 0) {
            OrbisPadVibeParam vibe = { 0, 0 };
            scePadSetVibration(padHandles[i], &vibe);
            scePadClose(padHandles[i]);
        }
    }
    send_vibe_to_daemon(0, 0);

    // Gracefully exit back to PS4 shell without CE-34878-0
    sceSystemServiceLoadExec("exit", NULL);
    exit(0);

    return 0;
}
