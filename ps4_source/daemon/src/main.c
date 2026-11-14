/*
 * OneFamily Daemon for PS4 (GoldHEN)
 * Entry Point and Service Manager
 */

#include "vpad.h"
#include "usb_driver.h"
#include "bt_driver.h"
#include "config.h"
#include "log.h"
#include "daemon_syscalls.h"
#include "generic_hid.h"
#include "onefamily_version.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

static volatile bool g_running = true;

static void check_vibe_file(void) {
    static uint8_t s_last_lg = 0;
    static uint8_t s_last_sm = 0;
    static int s_had_file = 0;

    int fd = daemon_sys_open("/data/anycontroller/anycontroller.vibe", 0 /* O_RDONLY */, 0);
    if (fd < 0) {
        fd = daemon_sys_open("/user/data/anycontroller/anycontroller.vibe", 0 /* O_RDONLY */, 0);
    }
    if (fd >= 0) {
        uint8_t vibe[2] = {0, 0};
        int n = daemon_sys_read(fd, vibe, sizeof(vibe));
        daemon_sys_close(fd);
        if (n >= 2) {
            s_had_file = 1;
            if (vibe[0] != s_last_lg || vibe[1] != s_last_sm) {
                s_last_lg = vibe[0];
                s_last_sm = vibe[1];
                usb_driver_send_rumble(s_last_lg, s_last_sm);
                bt_driver_send_rumble(s_last_lg, s_last_sm, NULL);
            }
        }
    } else {
        if (s_had_file) {
            s_had_file = 0;
            s_last_lg = 0;
            s_last_sm = 0;
            usb_driver_send_rumble(0, 0);
            bt_driver_send_rumble(0, 0, NULL);
        }
    }
}

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    // 1. Initialize Safe Kernel resolution and Logging
    daemon_log_init();
    daemon_log(LOG_LEVEL_INFO, "=== AnyController Daemon " ONEFAMILY_VERSION_STRING " (Native ES) Starting ===");
    daemon_kernel_init();

    // Ensure persistent storage and controller profile directories exist (/data and /user/data for FW 5.05)
    daemon_sys_mkdir("/data", 0777);
    daemon_sys_mkdir("/data/anycontroller", 0777);
    daemon_sys_mkdir("/data/anycontroller/controllers", 0777);
    daemon_sys_mkdir("/user", 0777);
    daemon_sys_mkdir("/user/data", 0777);
    daemon_sys_mkdir("/user/data/anycontroller", 0777);
    daemon_sys_mkdir("/user/data/anycontroller/controllers", 0777);

    // Purge any legacy onefamily files if present
    daemon_sys_unlink("/data/payloads/onefamily.bin");
    daemon_sys_unlink("/user/data/payloads/onefamily.bin");
    daemon_sys_unlink("/data/onefamily/onefamily.ini");
    daemon_sys_unlink("/user/data/onefamily/onefamily.ini");
    daemon_sys_unlink("/data/onefamily/onefamily.status");
    daemon_sys_unlink("/user/data/onefamily/onefamily.status");
    daemon_sys_unlink("/data/onefamily/onefamily.log");
    daemon_sys_unlink("/user/data/onefamily/onefamily.log");
    daemon_sys_unlink("/data/onefamily/onefamily.reload");
    daemon_sys_unlink("/user/data/onefamily/onefamily.reload");

    // 2. Load user configuration from /data/anycontroller/anycontroller.ini
    daemon_config_load(&g_daemon_config);
    daemon_log(LOG_LEVEL_INFO, "Configuration loaded: guide_action=%d, deadzone=%d, trigger=%d, rumble=%d",
               g_daemon_config.guide_action,
               g_daemon_config.stick_deadzone,
               g_daemon_config.trigger_threshold,
               g_daemon_config.enable_rumble);

    // 3. Notify user on screen immediately
    daemon_notify("AnyController " ONEFAMILY_VERSION_STRING " Activo (ES)");

    // 4. Initialize USB Driver Subsystem
    if (usb_driver_init() != 0) {
        daemon_log(LOG_LEVEL_ERROR, "Failed to initialize USB Driver");
    }

    // 5. Initialize DualSense Bluetooth Subsystem
    if (bt_driver_init() != 0) {
        daemon_log(LOG_LEVEL_INFO, "DualSense Bluetooth Subsystem skipped or not supported on this firmware");
    }

    daemon_log(LOG_LEVEL_INFO, "=== Daemon Ready - Entering USB & BT Polling Loop (250 Hz) ===");

    uint32_t loop_count = 0;

    // Main service loop (250 Hz)
    while (g_running) {
        uint64_t loop_start_us = daemon_sys_time_us();

        // Poll USB controller and feed Virtual DualShock 4
        usb_driver_poll_step();

        // Poll Bluetooth DualSense controller and feed Virtual DualShock 4
        bt_driver_poll_step();

        // Poll virtual DS4 rumble from OS/games
        vpad_poll_rumble();

        // Poll companion app vibration test IPC (~25 Hz = every 10 loops)
        if (++loop_count % 10 == 0) {
            check_vibe_file();
        }

        // Check if mapping reload or PS4 profile picker (.rebind) was requested (~5 Hz = every 50 loops)
        if (loop_count % 50 == 0) {
            int reload_fd = daemon_sys_open("/data/anycontroller/anycontroller.reload", 0 /* O_RDONLY */, 0);
            if (reload_fd < 0) {
                reload_fd = daemon_sys_open("/user/data/anycontroller/anycontroller.reload", 0 /* O_RDONLY */, 0);
            }
            if (reload_fd >= 0) {
                daemon_sys_close(reload_fd);
                daemon_sys_unlink("/data/anycontroller/anycontroller.reload");
                daemon_sys_unlink("/user/data/anycontroller/anycontroller.reload");
                if (g_usb_state.connected && g_usb_state.type == CONTROLLER_GENERIC_HID) {
                    generic_hid_reload(g_usb_state.vid, g_usb_state.pid);
                    daemon_notify("AnyController: Perfil HID Recargado");
                }
                usb_driver_reload_remap();
                daemon_config_load(&g_daemon_config);
                bt_driver_refresh_settings();
                vpad_apply_ds4_lightbar();
            }

            int rebind_fd = daemon_sys_open("/data/anycontroller/anycontroller.rebind", 0 /* O_RDONLY */, 0);
            if (rebind_fd < 0) {
                rebind_fd = daemon_sys_open("/user/data/anycontroller/anycontroller.rebind", 0 /* O_RDONLY */, 0);
            }
            if (rebind_fd >= 0) {
                char rbuf[16] = {0};
                int rn = daemon_sys_read(rebind_fd, rbuf, sizeof(rbuf) - 1);
                daemon_sys_close(rebind_fd);
                daemon_sys_unlink("/data/anycontroller/anycontroller.rebind");
                daemon_sys_unlink("/user/data/anycontroller/anycontroller.rebind");
                int target_slot = -1;
                if (rn > 0 && rbuf[0] >= '0' && rbuf[0] <= '3') {
                    target_slot = rbuf[0] - '0';
                }
                if (target_slot >= 0) {
                    vpad_reopen_user_picker_slot(target_slot);
                } else {
                    int opened = 0;
                    for (int s = 3; s >= 0; --s) {
                        if (vpad_slot_is_active(s)) {
                            vpad_reopen_user_picker_slot(s);
                            opened = 1;
                            break;
                        }
                    }
                    if (!opened) vpad_reopen_user_picker_slot(0);
                }
                bt_driver_write_status();
            }

            // Detect PS4 user profile logins (~5 Hz); only write status to disk when count changed
            if (vpad_check_user_rebind(g_daemon_config.mode)) {
                bt_driver_write_status();
            }
        }

        // Smooth 16 Hz animation when native PS4 DS4 is in Dynamic Rainbow (4) or Breathing (5) mode
        if (loop_count % 15 == 0) {
            if (!vpad_is_active() &&
                (g_daemon_config.ds4_lightbar_mode == 4 || g_daemon_config.ds4_lightbar_mode == 5)) {
                vpad_apply_ds4_lightbar();
            }
        }

        // Periodically check if configuration INI file was updated (every ~2s = 500 loops)
        if (loop_count % 500 == 0) {
            daemon_config_check_reload(&g_daemon_config);
            if (!vpad_is_active()) {
                vpad_apply_ds4_lightbar();
            }
        }

        // Precision sleep to pace exactly at 250 Hz (4000 us)
        uint64_t elapsed_us = daemon_sys_time_us() - loop_start_us;
        if (elapsed_us < 4000ULL) {
            daemon_sys_usleep((uint32_t)(4000ULL - elapsed_us));
        }
    }

    daemon_log(LOG_LEVEL_INFO, "Shutting down AnyController Daemon...");
    bt_driver_shutdown();
    usb_driver_shutdown();
    vpad_shutdown();
    daemon_log_close();

    return 0;
}
