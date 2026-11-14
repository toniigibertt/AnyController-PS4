#include "installer.h"
#include "config_manager.h"

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <ctype.h>
#include <orbis/libkernel.h>

#define BUNDLED_PAYLOAD_PATH         "/app0/assets/payload/anycontroller.bin"
#define BUNDLED_PAYLOAD_FALLBACK     "/app0/payload/anycontroller.bin"

#define TARGET_PAYLOAD_PATH          "/data/payloads/anycontroller.bin"
#define TARGET_USER_PAYLOAD_PATH     "/user/data/payloads/anycontroller.bin"
#define TARGET_LEGACY_PAYLOAD_PATH   "/data/payloads/onefamily.bin"
#define TARGET_LEGACY_USER_PAYLOAD   "/user/data/payloads/onefamily.bin"

#define PAYLOADS_INI_PATH            "/data/GoldHEN/payloads.ini"
#define PAYLOADS_INI_USER_PATH       "/user/data/GoldHEN/payloads.ini"

static bool file_exists(const char* path) {
    int fd = sceKernelOpen(path, O_RDONLY, 0);
    if (fd >= 0) {
        sceKernelClose(fd);
        return true;
    }
    return false;
}

static void unlink_all_files_in_dir(const char* dir_path) {
    int dfd = sceKernelOpen(dir_path, O_RDONLY, 0);
    if (dfd < 0) return;

    char dent_buf[2048];
    int nread = 0;
    while ((nread = sceKernelGetdents(dfd, dent_buf, sizeof(dent_buf))) > 0) {
        int pos = 0;
        while (pos < nread) {
            struct dirent* d = (struct dirent*)(dent_buf + pos);
            if (d->d_reclen == 0) break;
            const char* name = d->d_name;
            if (strcmp(name, ".") != 0 && strcmp(name, "..") != 0) {
                char full_path[256];
                snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, name);
                sceKernelUnlink(full_path);
                rmdir(full_path);
            }
            pos += d->d_reclen;
        }
    }
    sceKernelClose(dfd);
}

void Installer::cleanupLegacyOneFamily() {
    sceKernelUnlink(TARGET_LEGACY_PAYLOAD_PATH);
    sceKernelUnlink(TARGET_LEGACY_USER_PAYLOAD);
    sceKernelUnlink("/data/payloads/xbox_controller.bin");
    sceKernelUnlink("/user/data/payloads/xbox_controller.bin");
    sceKernelUnlink("/data/GoldHEN/onefamily.ini");
    sceKernelUnlink("/user/data/GoldHEN/onefamily.ini");
    sceKernelUnlink("/data/GoldHEN/xbox_controller.ini");
    sceKernelUnlink("/user/data/GoldHEN/xbox_controller.ini");
    sceKernelUnlink("/data/GoldHEN/onefamily.log");
    sceKernelUnlink("/user/data/GoldHEN/onefamily.log");
    sceKernelUnlink("/data/GoldHEN/xbox_controller.log");
    sceKernelUnlink("/user/data/GoldHEN/xbox_controller.log");
    sceKernelUnlink("/data/GoldHEN/onefamily.vibe");
    sceKernelUnlink("/user/data/GoldHEN/onefamily.vibe");
    sceKernelUnlink("/data/pkg/onefamily_companion.pkg");
    sceKernelUnlink("/user/data/pkg/onefamily_companion.pkg");

    unlink_all_files_in_dir("/data/onefamily/controllers");
    unlink_all_files_in_dir("/data/onefamily/maps");
    unlink_all_files_in_dir("/data/onefamily");
    rmdir("/data/onefamily/controllers");
    rmdir("/data/onefamily/maps");
    rmdir("/data/onefamily");

    unlink_all_files_in_dir("/user/data/onefamily/controllers");
    unlink_all_files_in_dir("/user/data/onefamily/maps");
    unlink_all_files_in_dir("/user/data/onefamily");
    rmdir("/user/data/onefamily/controllers");
    rmdir("/user/data/onefamily/maps");
    rmdir("/user/data/onefamily");
}

static bool read_entire_file(const char* path, std::vector<uint8_t>& out) {
    out.clear();
    int fd = sceKernelOpen(path, O_RDONLY, 0);
    if (fd < 0) return false;

    off_t size = sceKernelLseek(fd, 0, SEEK_END);
    if (size <= 0) {
        sceKernelClose(fd);
        return false;
    }
    sceKernelLseek(fd, 0, SEEK_SET);

    out.resize((size_t)size);
    size_t total = 0;
    while (total < (size_t)size) {
        ssize_t rd = sceKernelRead(fd, out.data() + total, (size_t)size - total);
        if (rd <= 0) break;
        total += rd;
    }
    sceKernelClose(fd);
    return (total == (size_t)size);
}

static bool read_text_file(const char* path, std::string& out) {
    out.clear();
    std::vector<uint8_t> buf;
    if (!read_entire_file(path, buf)) return false;
    out.assign((const char*)buf.data(), buf.size());
    return true;
}

static bool write_text_file(const char* path, const std::string& content) {
    int fd = sceKernelOpen(path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0) return false;
    size_t total = 0;
    while (total < content.size()) {
        ssize_t wr = sceKernelWrite(fd, content.data() + total, content.size() - total);
        if (wr <= 0) break;
        total += wr;
    }
    sceKernelClose(fd);
    return (total == content.size());
}

static bool write_binary_file(const char* path, const void* data, size_t size) {
    int fd = sceKernelOpen(path, O_WRONLY | O_CREAT | O_TRUNC, 0777);
    if (fd < 0) return false;
    size_t total = 0;
    const uint8_t* ptr = (const uint8_t*)data;
    while (total < size) {
        ssize_t wr = sceKernelWrite(fd, ptr + total, size - total);
        if (wr <= 0) break;
        total += wr;
    }
    sceKernelClose(fd);
    return (total == size);
}

static bool update_payloads_ini() {
    std::string content;
    bool found = read_text_file(PAYLOADS_INI_PATH, content);
    if (!found) {
        found = read_text_file(PAYLOADS_INI_USER_PATH, content);
    }

    const std::string entryLine = "/data/payloads/anycontroller.bin = 1";

    if (!found || content.empty()) {
        // File does not exist yet; create it with default [AutoRun] section
        std::string newIni = "[AutoRun]\n" + entryLine + "\n";
        write_text_file(PAYLOADS_INI_PATH, newIni);
        write_text_file(PAYLOADS_INI_USER_PATH, newIni);
        return true;
    }

    bool modified = false;

    // Remove any obsolete legacy xbox_controller.bin or onefamily.bin autorun lines so we don't double-launch
    const char* obsoleteNames[2] = { "xbox_controller.bin", "onefamily.bin" };
    for (int k = 0; k < 2; k++) {
        size_t xpos = 0;
        while ((xpos = content.find(obsoleteNames[k])) != std::string::npos) {
            size_t lineStart = content.rfind('\n', xpos);
            if (lineStart == std::string::npos) lineStart = 0;
            else lineStart++; // move past '\n'
            size_t lineEnd = content.find('\n', xpos);
            if (lineEnd == std::string::npos) {
                content.erase(lineStart);
            } else {
                content.erase(lineStart, lineEnd - lineStart + 1);
            }
            modified = true;
        }
    }

    // Migrate legacy /user/data/payloads/anycontroller.bin to canonical /data/payloads/anycontroller.bin
    const std::string legacyEntry = "/user/data/payloads/anycontroller.bin";
    size_t legacyPos = content.find(legacyEntry);
    if (legacyPos != std::string::npos) {
        content.replace(legacyPos, legacyEntry.length(), "/data/payloads/anycontroller.bin");
        modified = true;
    }

    // Ensure anycontroller.bin is present in [AutoRun]
    if (content.find("anycontroller.bin") == std::string::npos) {
        // Find [AutoRun] section (case-insensitive)
        std::string lower = content;
        for (size_t i = 0; i < lower.size(); i++) {
            lower[i] = (char)::tolower((unsigned char)lower[i]);
        }

        size_t pos = lower.find("[autorun]");
        if (pos != std::string::npos) {
            // Insert right after the [AutoRun] line
            size_t eol = content.find('\n', pos);
            if (eol == std::string::npos) {
                content += "\n" + entryLine + "\n";
            } else {
                content.insert(eol + 1, entryLine + "\n");
            }
        } else {
            // Section not present; append to end
            if (!content.empty() && content.back() != '\n') {
                content += "\n";
            }
            content += "\n[AutoRun]\n" + entryLine + "\n";
        }
        modified = true;
    }

    if (modified) {
        write_text_file(PAYLOADS_INI_PATH, content);
        write_text_file(PAYLOADS_INI_USER_PATH, content);
    }
    return true;
}

bool Installer::isPayloadUpToDate() {
    // 1. Locate bundled payload
    const char* bundled_path = NULL;
    if (file_exists(BUNDLED_PAYLOAD_PATH)) {
        bundled_path = BUNDLED_PAYLOAD_PATH;
    } else if (file_exists(BUNDLED_PAYLOAD_FALLBACK)) {
        bundled_path = BUNDLED_PAYLOAD_FALLBACK;
    }
    if (!bundled_path) return false;

    // 2. Locate installed target payload
    const char* target_path = NULL;
    if (file_exists(TARGET_PAYLOAD_PATH)) {
        target_path = TARGET_PAYLOAD_PATH;
    } else if (file_exists(TARGET_USER_PAYLOAD_PATH)) {
        target_path = TARGET_USER_PAYLOAD_PATH;
    }
    if (!target_path) return false;

    // 3. Open both and compare file sizes
    int fd_b = sceKernelOpen(bundled_path, O_RDONLY, 0);
    if (fd_b < 0) return false;
    off_t bundled_sz = sceKernelLseek(fd_b, 0, SEEK_END);
    sceKernelLseek(fd_b, 0, SEEK_SET);

    int fd_t = sceKernelOpen(target_path, O_RDONLY, 0);
    if (fd_t < 0) {
        sceKernelClose(fd_b);
        return false;
    }
    off_t target_sz = sceKernelLseek(fd_t, 0, SEEK_END);
    sceKernelLseek(fd_t, 0, SEEK_SET);

    if (bundled_sz <= 0 || target_sz <= 0 || bundled_sz != target_sz) {
        sceKernelClose(fd_b);
        sceKernelClose(fd_t);
        return false;
    }

    // 4. Compare contents in 8 KB chunks
    uint8_t buf_b[8192];
    uint8_t buf_t[8192];
    bool match = true;
    off_t remaining = bundled_sz;

    while (remaining > 0) {
        size_t chunk = (remaining > (off_t)sizeof(buf_b)) ? sizeof(buf_b) : (size_t)remaining;
        ssize_t rb = sceKernelRead(fd_b, buf_b, chunk);
        ssize_t rt = sceKernelRead(fd_t, buf_t, chunk);
        if (rb != (ssize_t)chunk || rt != (ssize_t)chunk || memcmp(buf_b, buf_t, chunk) != 0) {
            match = false;
            break;
        }
        remaining -= chunk;
    }

    sceKernelClose(fd_b);
    sceKernelClose(fd_t);
    return match;
}

bool Installer::isInstalled() {
    // 1. Target payload must exist
    bool binExists = file_exists(TARGET_PAYLOAD_PATH) || file_exists(TARGET_USER_PAYLOAD_PATH);
    if (!binExists) return false;

    // 2. payloads.ini must exist and mention anycontroller.bin
    std::string content;
    bool iniExists = read_text_file(PAYLOADS_INI_PATH, content);
    if (!iniExists) {
        iniExists = read_text_file(PAYLOADS_INI_USER_PATH, content);
    }
    if (!iniExists) return false;

    if (content.find("anycontroller.bin") == std::string::npos) {
        return false;
    }

    // Migrate legacy /user/data/payloads entry to /data/payloads in-place if needed
    if (content.find("/user/data/payloads/anycontroller.bin") != std::string::npos ||
        content.find("onefamily.bin") != std::string::npos) {
        update_payloads_ini();
    }

    return true;
}

#include "factory_profiles.h"

static void upgrade_ini_file_if_needed(const char* path) {
    std::string content;
    if (!read_text_file(path, content) || content.empty()) {
        return;
    }

    std::string lower = content;
    for (size_t i = 0; i < lower.size(); i++) {
        lower[i] = (char)::tolower((unsigned char)lower[i]);
    }

    // Check if [Rumble] section or rumble key is already present
    if (lower.find("[rumble]") != std::string::npos ||
        lower.find("\nrumble ") != std::string::npos ||
        lower.find("\nrumble=") != std::string::npos ||
        lower.find("\nrumble_report") != std::string::npos) {
        return;
    }

    if (content.back() != '\n') {
        content += "\n";
    }
    content += "\n[Rumble]\n"
               "# Presets: auto, logitech, dragonrise, greenasia, betop, shanwan, 8bitdo, dinput05, none (or hex e.g. 05 L R 00 00)\n"
               "rumble = auto\n";
    write_text_file(path, content);
}

static void upgrade_existing_profiles_in_dir(const char* dir_path) {
    int dfd = sceKernelOpen(dir_path, O_RDONLY, 0);
    if (dfd < 0) return;

    char dent_buf[2048];
    int nread = 0;
    while ((nread = sceKernelGetdents(dfd, dent_buf, sizeof(dent_buf))) > 0) {
        int pos = 0;
        while (pos < nread) {
            struct dirent* d = (struct dirent*)(dent_buf + pos);
            if (d->d_reclen == 0) break;
            const char* name = d->d_name;
            size_t nlen = strlen(name);
            if (nlen > 4 && strcasecmp(name + nlen - 4, ".ini") == 0) {
                char full_path[256];
                snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, name);
                upgrade_ini_file_if_needed(full_path);
            }
            pos += d->d_reclen;
        }
    }
    sceKernelClose(dfd);
}

static void install_controller_profiles() {
    const char* profileDirs[2] = {
        "/data/anycontroller/controllers",
        "/user/data/anycontroller/controllers"
    };
    for (size_t i = 0; i < sizeof(k_factory_profiles) / sizeof(k_factory_profiles[0]); i++) {
        for (int d = 0; d < 2; d++) {
            char path[128];
            snprintf(path, sizeof(path), "%s/%s", profileDirs[d], k_factory_profiles[i].filename);
            if (!file_exists(path)) {
                write_text_file(path, k_factory_profiles[i].content);
            } else {
                upgrade_ini_file_if_needed(path);
            }
        }
    }

    // Also upgrade any user-created <VID>_<PID>.ini profiles in the directories
    for (int d = 0; d < 2; d++) {
        upgrade_existing_profiles_in_dir(profileDirs[d]);
    }
}

InstallResult Installer::runInstallation(bool isUpdate) {
    InstallResult res;
    res.isUpdate = isUpdate;

    // 1. Ensure AnyController target directory tree ONLY (never create onefamily)
    mkdir("/data", 0777);
    mkdir("/data/payloads", 0777);
    mkdir("/data/GoldHEN", 0777);
    mkdir("/data/anycontroller", 0777);
    mkdir("/data/anycontroller/controllers", 0777);
    mkdir("/user", 0777);
    mkdir("/user/data", 0777);
    mkdir("/user/data/payloads", 0777);
    mkdir("/user/data/GoldHEN", 0777);
    mkdir("/user/data/anycontroller", 0777);
    mkdir("/user/data/anycontroller/controllers", 0777);

    // 2. Read bundled payload from PKG
    std::vector<uint8_t> payloadData;
    bool readOk = read_entire_file(BUNDLED_PAYLOAD_PATH, payloadData);
    if (!readOk) {
        readOk = read_entire_file(BUNDLED_PAYLOAD_FALLBACK, payloadData);
    }

    if (!readOk || payloadData.empty()) {
        res.detailMessage = "No se encontro anycontroller.bin en /app0/assets/payload/";
        return res;
    }

    // 3. Write payload ONLY to /data/payloads/anycontroller.bin
    sceKernelUnlink(TARGET_PAYLOAD_PATH);
    sceKernelUnlink(TARGET_USER_PAYLOAD_PATH);
    bool write1 = write_binary_file(TARGET_PAYLOAD_PATH, payloadData.data(), payloadData.size());
    bool write2 = write_binary_file(TARGET_USER_PAYLOAD_PATH, payloadData.data(), payloadData.size());
    res.payloadCopied = (write1 || write2);
    if (!res.payloadCopied) {
        res.detailMessage = "Error al escribir el payload en /data/payloads/anycontroller.bin";
        return res;
    }

    // 4. Initialize configuration file in /data/anycontroller/anycontroller.ini
    ConfigManager cfgMgr;
    cfgMgr.load();
    cfgMgr.save();

    // Install default generic controller profiles in /data/anycontroller/controllers/
    install_controller_profiles();

    // Purge all legacy onefamily files, folders, and binaries
    cleanupLegacyOneFamily();
    res.configCreated = true;

    // 5. Update /data/GoldHEN/payloads.ini for permanent autoboot
    res.autobootConfigured = update_payloads_ini();
    if (!res.autobootConfigured) {
        res.detailMessage = "Error al configurar /data/GoldHEN/payloads.ini";
        return res;
    }

    return res;
}
