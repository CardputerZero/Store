/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

#include "json.hpp"
#ifdef APPSTORE_NATIVE_IN_PROCESS_TEST
#include "appstore_native.hpp"
#include <thread>
namespace appstore { std::string native_test_sync_registry(const std::string &, const std::string &); }
#endif
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

using json = nlohmann::json;
namespace fs = std::filesystem;
struct Result { int rc; std::string out; };
static fs::path root, binary;
static int checks;
#define CHECK(x) do { ++checks; if (!(x)) { std::cerr << "FAIL " << __LINE__ << ": " #x "\n"; std::exit(1); } } while (0)

static void text(const fs::path &p, const std::string &s, mode_t mode = 0644) {
    fs::create_directories(p.parent_path()); std::ofstream f(p); f << s; f.close(); CHECK(f.good()); chmod(p.c_str(), mode);
}
static void save(const fs::path &p, const json &v) { text(p, v.dump(2) + "\n"); }
static json load(const fs::path &p) { std::ifstream f(p); json v; f >> v; return v; }
static bool has(const Result &r, const std::string &s) { return r.out.find(s) != std::string::npos; }

static Result call(std::initializer_list<std::string> args) {
    int fd[2]; CHECK(pipe(fd) == 0); pid_t pid = fork(); CHECK(pid >= 0);
    if (!pid) {
        close(fd[0]); dup2(fd[1], 1); dup2(fd[1], 2); close(fd[1]);
        std::vector<std::string> values{binary.string()}; values.insert(values.end(), args);
        std::vector<char *> argv; for (auto &v : values) argv.push_back(v.data()); argv.push_back(nullptr);
        execv(argv[0], argv.data()); _exit(127);
    }
    close(fd[1]); std::string out; char buf[4096]; ssize_t n;
    while ((n = read(fd[0], buf, sizeof(buf))) > 0) out.append(buf, n);
    close(fd[0]);
    int status; CHECK(waitpid(pid, &status, 0) == pid); return {WIFEXITED(status) ? WEXITSTATUS(status) : 255, out};
}
static void call_and_kill(std::initializer_list<std::string> args) {
    pid_t pid=fork(); CHECK(pid>=0);
    if (!pid) {
        std::vector<std::string> values{binary.string()}; values.insert(values.end(),args);
        std::vector<char *> argv; for(auto &v:values) argv.push_back(v.data()); argv.push_back(nullptr);
        execv(argv[0],argv.data()); _exit(127);
    }
    usleep(200000); CHECK(kill(pid,SIGKILL)==0); int status=0; CHECK(waitpid(pid,&status,0)==pid&&WIFSIGNALED(status));
}
static std::string tx(const Result &r) {
    std::istringstream lines(r.out); std::string line;
    while (std::getline(lines, line)) {
        if (line.rfind("PACKAGE_JOB\t", 0) == 0) {
            std::istringstream row(line); std::string f; std::vector<std::string> fields;
            while (std::getline(row, f, '\t')) fields.push_back(f);
            return fields.size() > 5 ? fields[5] : "";
        }
    }
    return "";
}
static json app(const std::string &id, const std::string &review) {
    json result={{"uuid",id},{"share_code",id=="app-id"?"demo":"blocked"},{"title",id=="app-id"?"Demo":"Blocked"},
      {"version","1.2.0"},{"review_status",review},{"featured",id=="app-id"},{"categories",{"Utilities","Tools"}},
      {"i18n",{{"en",{{"title","Demo Local"},{"summary","Localized summary"}}},{"zh-CN",{{"title","演示"},{"summary","本地化摘要"}}}}},
      {"source",{{"repository","https://example.invalid/source"}}},
      {"description","Full detailed description"},
      {"icon","https://assets.invalid/"+id+".png"},
      {"app",{{"dependencies",{"lib-one","lib-two"}},{"applaunch",{{"desktop_entry","applications/demo.desktop"},{"exec","/usr/bin/demo-package"}}}}},
      {"permissions",{{"network",true},{"camera",id=="app-id"},{"microphone",false}}},
      {"download",{{"type","deb"},{"package","demo-package"},{"url","https://example.invalid/demo.deb"},{"md5",getenv("FAKE_MD5")},{"size","42K"}}}};
    result["author"] = id == "app-id"
        ? json{{"display_name", "Tester"}, {"github", "ignored-user"}}
        : json{{"github", "fallback-user"}, {"name", "ignored-name"}};
    return result;
}
static void fixture() {
    fs::create_directories(root/"bin"); fs::create_directories(root/"state"); fs::create_directories(root/"cache"); fs::create_directories(root/"root/applications");
    text(root/"demo.deb", "fake deb payload\n");
    std::string cmd="md5sum '"+(root/"demo.deb").string()+"'"; FILE *p=popen(cmd.c_str(),"r"); char md5[40]={}; CHECK(fgets(md5,sizeof(md5),p)); pclose(p); md5[32]=0; setenv("FAKE_MD5",md5,1);
    save(root/"registry.json", {{"apps",{app("app-id","approved"),app("blocked-id","pending")}}});
    text(root/"bin/curl", R"(#!/bin/sh
set -eu; out=; url=
while [ $# -gt 0 ]; do case "$1" in -o) out=$2;shift 2;; -A|--connect-timeout|--max-time|-C) shift 2;; -*) shift;; *) url=$1;shift;; esac; done
printf '%s\n' "$url" >> "$M5APPSTORE_CACHE_DIR/network.log"
[ "${FAKE_CURL_FAIL:-0}" = 0 ] || exit 22
case "$url" in
  https://cardputerzero.github.io/generated/registry.json*) outcome=${FAKE_DEFAULT_OUTCOME:-ok};;
  https://cardputer-zero-repo.oss-cn-shenzhen.aliyuncs.com/packages/cn/registry.json*) outcome=${FAKE_CN_OUTCOME:-ok};;
  *) outcome=ok;;
esac
case "$outcome" in
  fail) exit 7;;
  invalid) printf '{invalid' > "$out"; exit 0;;
  bad) printf '{"error":"not a catalog"}' > "$out"; exit 0;;
  wrong-apps) printf '{"apps":{}}' > "$out"; exit 0;;
  bad-app) printf '{"apps":[{"uuid":42}]}' > "$out"; exit 0;;
  empty) printf '{"apps":[]}' > "$out"; exit 0;;
  slow) sleep 4;;
esac
[ -z "${FAKE_CURL_SLEEP:-}" ] || sleep "$FAKE_CURL_SLEEP"
marker="$M5APPSTORE_CACHE_DIR/network-drop-marker"
if [ "${FAKE_DROP_DURING_ASSETS:-0}" = 1 ] && [ -e "$marker" ]; then exit 7; fi
case "$url" in
  *ipinfo.io*) exit 99;;
  *demo.deb*) cp "$FAKE_DEB" "$out";;
  *.png*) [ -z "${FAKE_ASSET_SLEEP:-}" ] || sleep "$FAKE_ASSET_SLEEP"
          [ "${FAKE_ASSET_FAIL:-0}" = 0 ] || exit 22; printf 'fake icon' > "$out";;
  *) case "$url" in *oss-cn-shenzhen*) cp "${FAKE_CN_REGISTRY:-$FAKE_REGISTRY}" "$out";; *) cp "$FAKE_REGISTRY" "$out";; esac
     if [ "${FAKE_DROP_DURING_ASSETS:-0}" = 1 ]; then touch "$marker"; fi;;
esac
)",0755);
    text(root/"bin/dpkg-query", R"(#!/bin/sh
set -eu
if [ "$1" = -L ]; then [ "${FAKE_FILES_FAIL:-0}" = 0 ] || exit 2; printf '/usr/bin/demo-package\n/usr/share/APPLaunch/applications/demo.desktop\n'; exit; fi
case "${FAKE_DPKG_MODE:-absent}" in installed) printf '%s\t%s\n' "${FAKE_STATUS:-ii }" "${FAKE_VERSION:-1.2.0}";; un) printf 'un \t0.1.0\n';; error) echo 'database error' >&2;exit 2;; *) echo 'no packages found matching' >&2;exit 1;; esac
)",0755);
    text(root/"bin/dpkg-deb", R"(#!/bin/sh
case "$3" in Package) echo "${FAKE_DEB_PACKAGE:-demo-package}";; Version) echo "${FAKE_DEB_VERSION:-1.2.0}";; Pre-Depends|Depends) echo -n "${FAKE_DEPENDS:-}";; *) exit 1;; esac
)",0755);
    std::string path=(root/"bin").string()+":"+getenv("PATH"); setenv("PATH",path.c_str(),1);
    setenv("FAKE_DEB",(root/"demo.deb").c_str(),1); setenv("FAKE_REGISTRY",(root/"registry.json").c_str(),1);
    setenv("M5APPSTORE_STATE_DIR",(root/"state").c_str(),1); setenv("M5APPSTORE_CACHE_DIR",(root/"cache").c_str(),1);
    setenv("M5APPSTORE_APP_ROOT",(root/"root").c_str(),1); setenv("M5APPSTORE_HTTP_TRANSPORT","curl",1);
    setenv("M5APPSTORE_LOCALE","en",1);
}
static void registry_cases() {
    CHECK(has(call({"--regions"}),"REGION\tauto\tAuto"));
    CHECK(call({"--set-region","moon"}).rc!=0); CHECK(call({"--set-region","CN"}).rc==0);
    CHECK(has(call({"--registry-config"}),"CONFIG\tCN\tCN\t1")); CHECK(call({"--set-region","default"}).rc==0);
    Result s=call({"--sync"}); CHECK(s.rc==0&&has(s,"SYNC\t1\t0\t0\t2"));
    Result sum=call({"--summary"}); CHECK(has(sum,"META\t1\t2 apps/1 registries"));
    CHECK(has(sum,"CAT\tAll\nCAT\tInstalled\nCAT\tUtilities\nCAT\tTools\n"));
    CHECK(!has(sum,"CAT\tRecommended\n"));
    CHECK(has(sum,"APP\tapp-id\tDemo Local\t1.2.0\tUtilities\t0\t1\t"));
    CHECK(has(sum,"APP\tblocked-id\tDemo Local\t1.2.0\tUtilities\t0\t0\t"));
    CHECK(has(sum,"\tapproved\t1\t")); CHECK(has(sum,"\tpending\t0\t"));
    CHECK(has(sum,"\tTester\thttps://example.invalid/source\t"));
    CHECK(has(sum,"Localized summary\\nFull detailed description"));
    // Screenshot count, then the permissions bit set: network (1) and camera (4).
    CHECK(has(sum,"\tdemo-package\t0\t5"));
    const json original_registry = load(root/"registry.json");
    json legacy_categories = original_registry;
    legacy_categories["apps"][0]["categories"].push_back("Recommended");
    save(root/"registry.json", legacy_categories);
    CHECK(call({"--sync"}).rc==0);
    Result legacy_summary=call({"--summary"});
    CHECK(!has(legacy_summary,"CAT\tRecommended\n"));
    CHECK(has(legacy_summary,"\tUtilities\x1fTools\x1fRecommended\t"));
    save(root/"registry.json", original_registry);
    CHECK(call({"--clear-registry-cache"}).rc==0);
    setenv("FAKE_DROP_DURING_ASSETS","1",1);
    Result dropped=call({"--sync"});
    CHECK(has(dropped,"SYNC\t1\t0\t0\t2\tCatalog synced"));
    unsetenv("FAKE_DROP_DURING_ASSETS");
    fs::remove(root/"cache/network-drop-marker");
    CHECK(has(call({"--sync"}),"SYNC\t1\t0\t0\t2"));
    CHECK(call({"--clear-registry-cache"}).rc==0);
    setenv("FAKE_ASSET_FAIL","1",1);
    CHECK(has(call({"--sync"}),"SYNC\t1\t0\t0\t2\tCatalog synced"));
    unsetenv("FAKE_ASSET_FAIL");
    CHECK(has(call({"--sync"}),"SYNC\t1\t0\t0\t2"));
    setenv("M5APPSTORE_LOCALE","zh_CN.UTF-8",1); CHECK(has(call({"--summary"}),"APP\tapp-id\t演示")); setenv("M5APPSTORE_LOCALE","en",1);
    Result author_summary=call({"--summary"});
    CHECK(has(author_summary,"\tTester\t"));
    CHECK(has(author_summary,"\tfallback-user\t"));
    CHECK(!has(author_summary,"\tignored-user\t"));
    CHECK(!has(author_summary,"\tignored-name\t"));
    json registry_with_repo_author = load(root/"registry.json");
    json repo_author_app = app("repo-author-id", "approved");
    repo_author_app["author"] = json::object();
    repo_author_app.erase("source");
    repo_author_app["source_repo"] = "https://github.com/repo-owner/example";
    registry_with_repo_author["apps"].push_back(repo_author_app);
    save(root/"registry.json", registry_with_repo_author);
    CHECK(call({"--sync"}).rc==0);
    CHECK(has(call({"--summary"}),"\trepo-owner\t"));
    registry_with_repo_author["apps"].erase(
        registry_with_repo_author["apps"].end() - 1);
    save(root/"registry.json", registry_with_repo_author);
    CHECK(call({"--sync"}).rc==0);
    CHECK(has(call({"--registries"}),"\tok\t2\t")); setenv("FAKE_CURL_FAIL","1",1);
    CHECK(has(call({"--sync"}),"SYNC\t0\t1\t0\t2")); CHECK(has(call({"--summary"}),"WARN\tRegistry offline")); unsetenv("FAKE_CURL_FAIL");
    const json original_config = load(root/"state/registries.json");
    text(root/"cache/network.log", "");
    for (const char *command : {"--add-registry", "--edit-registry", "--remove-registry", "--enable-registry", "--disable-registry"}) {
        Result rejected = call({command,"https://custom.invalid/registry.json","https://new.invalid/registry.json"});
        CHECK(rejected.rc!=0 && has(rejected,"custom registries are not supported"));
        CHECK(load(root/"state/registries.json")==original_config);
    }
    CHECK(fs::file_size(root/"cache/network.log")==0);
    CHECK(call({"--remove-registry","https://cardputerzero.github.io/generated/registry.json"}).rc!=0);
    json cfg={{"region","default"},{"active_region","default"},{"registries",{{{"name","Custom"},{"url","https://custom.invalid/registry.json"},{"enabled",false}}}}};
    Result imported = call({"--replace-registry-config",cfg.dump()});
    CHECK(imported.rc==0 && has(imported,"CONFIG\tdefault\tdefault\t1") && !has(imported,"Custom"));
    CHECK(call({"--replace-registry-config","{"}).rc!=0);
    fs::path c=root/"state/registries.json"; chmod(c.c_str(),0640); CHECK(call({"--set-region","CN"}).rc==0); struct stat st{}; CHECK(!stat(c.c_str(),&st)&&(st.st_mode&0777)==0640);
    CHECK(call({"--set-region","default"}).rc==0); CHECK(call({"--sync"}).rc==0);
}
static std::vector<std::string> catalog_requests() {
    std::ifstream input(root/"cache/network.log");
    std::vector<std::string> requests;
    std::string line;
    while (std::getline(input, line)) {
        CHECK(line.find("ipinfo") == std::string::npos);
        if (line.find("registry.json") != std::string::npos) requests.push_back(line.substr(0, line.find('?')));
    }
    return requests;
}
static void source_selection_cases() {
    const std::string github="https://cardputerzero.github.io/generated/registry.json";
    const std::string oss="https://cardputer-zero-repo.oss-cn-shenzhen.aliyuncs.com/packages/cn/registry.json";
    auto sync = [&]() { text(root/"cache/network.log", ""); return call({"--sync"}); };
    auto selected = [&](const std::string &mode, const std::string &active) {
        Result result=call({"--registry-config"});
        CHECK(result.rc==0 && has(result,"CONFIG\t"+mode+"\t"+active+"\t1\n"));
        CHECK(has(result,"CONFIG_REG\t0\tCardputerZero Hub"));
        CHECK(has(result,(active=="CN"?oss:github)+"\t1\t1\t"+active));
        CHECK(!has(result,"CONFIG_REG\t1"));
    };
    for (const std::string mode : {"auto", "default", "CN"}) {
        for (const json &active : {json("CN"), json("default"), json("moon"), json(12), json(nullptr)}) {
            json legacy={{"region",mode},{"active_region",active},
                {"registries",json::array({{{"url",github},{"enabled",false},{"name","Spoofed"}},
                    {{"url",oss},{"builtin",true}},{{"url","https://custom.invalid/x"},{"enabled",true}},42})}};
            const std::string expected=mode=="auto" ? (active=="CN"?"CN":"default") : mode;
            save(root/"state/registries.json",legacy);
            selected(mode,expected);
            CHECK(call({"--replace-registry-config",legacy.dump()}).rc==0);
            selected(mode,expected);
            json normalized=load(root/"state/registries.json");
            CHECK(normalized["registries"].size()==1 && normalized["registries"][0]["enabled"]==true);
        }
    }
    for (const json &mode : {json("moon"),json(42),json(nullptr)}) {
        json legacy={{"region",mode},{"active_region","moon"},{"registries",nullptr}};
        save(root/"state/registries.json",legacy); selected("auto","default");
        CHECK(call({"--replace-registry-config",legacy.dump()}).rc==0); selected("auto","default");
    }
    CHECK(call({"--replace-registry-config","[]"}).rc!=0);
    json cn_app=app("cn-only","approved"); cn_app["icon"]="";
    save(root/"cn-registry.json",{{"apps",{cn_app}}});
    setenv("FAKE_CN_REGISTRY",(root/"cn-registry.json").c_str(),1);
    CHECK(call({"--set-region","auto"}).rc==0);
    CHECK(has(sync(),"SYNC\t1\t0\t0\t2"));
    CHECK(catalog_requests()==std::vector<std::string>{github}); selected("auto","default");

    setenv("FAKE_DEFAULT_OUTCOME","fail",1);
    CHECK(has(sync(),"SYNC\t1\t0\t0\t1"));
    CHECK((catalog_requests()==std::vector<std::string>{github,oss})); selected("auto","CN");
    Result summary=call({"--summary"}); CHECK(has(summary,"APP\tcn-only") && !has(summary,"APP\tapp-id"));
    setenv("FAKE_CN_OUTCOME","fail",1);
    Result offline=sync(); CHECK(offline.rc!=0 && has(offline,"SYNC\t0\t1\t0\t1\tNo registry could be reached"));
    CHECK((catalog_requests()==std::vector<std::string>{github,oss})); selected("auto","CN");
    CHECK(has(call({"--summary"}),"WARN\tRegistry offline"));
    CHECK(call({"--clear-registry-cache"}).rc==0);
    CHECK(has(sync(),"SYNC\t0\t0\t1\t0\tNo registry could be reached"));
    CHECK(has(sync(),"SYNC\t0\t0\t1\t0\tNo registry could be reached"));
    unsetenv("FAKE_CN_OUTCOME");
    for (const char *bad : {"invalid", "bad", "wrong-apps", "bad-app"}) {
        setenv("FAKE_DEFAULT_OUTCOME",bad,1);
        CHECK(has(sync(),"SYNC\t1\t0\t0\t1"));
        CHECK((catalog_requests()==std::vector<std::string>{github,oss})); selected("auto","CN");
    }
    setenv("FAKE_DEFAULT_OUTCOME","empty",1);
    CHECK(has(sync(),"SYNC\t1\t0\t0\t0")); selected("auto","default");
    CHECK(catalog_requests()==std::vector<std::string>{github});
    unsetenv("FAKE_DEFAULT_OUTCOME");
    CHECK(has(sync(),"SYNC\t1\t0\t0\t2")); selected("auto","default");
    CHECK(catalog_requests()==std::vector<std::string>{github});
    const json original_catalog=load(root/"registry.json");
    json malformed_artwork=original_catalog;
    malformed_artwork["apps"][0]["assets"]=42;
    malformed_artwork["apps"][0]["icon"]=false;
    save(root/"registry.json",malformed_artwork);
    CHECK(has(sync(),"SYNC\t1\t0\t0\t2")); selected("auto","default");
    CHECK(catalog_requests()==std::vector<std::string>{github});
    save(root/"registry.json",original_catalog);

    CHECK(call({"--set-region","default"}).rc==0);
    setenv("FAKE_DEFAULT_OUTCOME","fail",1);
    CHECK(has(sync(),"SYNC\t0\t1\t0\t2"));
    CHECK(catalog_requests()==std::vector<std::string>{github}); selected("default","default");
    CHECK(call({"--set-region","CN"}).rc==0);
    CHECK(has(sync(),"SYNC\t1\t0\t0\t1"));
    CHECK(catalog_requests()==std::vector<std::string>{oss}); selected("CN","CN");
    unsetenv("FAKE_DEFAULT_OUTCOME"); setenv("FAKE_CN_OUTCOME","bad",1);
    CHECK(has(sync(),"SYNC\t0\t1\t0\t1"));
    CHECK(catalog_requests()==std::vector<std::string>{oss}); selected("CN","CN");
    unsetenv("FAKE_CN_OUTCOME");

    CHECK(call({"--set-region","auto"}).rc==0);
    setenv("FAKE_DEFAULT_OUTCOME","slow",1); setenv("M5APPSTORE_CATALOG_TIMEOUT","1",1);
    const auto started=std::chrono::steady_clock::now();
    CHECK(has(sync(),"SYNC\t1\t0\t0\t1"));
    CHECK(std::chrono::steady_clock::now()-started < std::chrono::seconds(4));
    CHECK((catalog_requests()==std::vector<std::string>{github,oss})); selected("auto","CN");
    unsetenv("FAKE_DEFAULT_OUTCOME"); unsetenv("M5APPSTORE_CATALOG_TIMEOUT");
    CHECK(call({"--clear-registry-cache"}).rc==0);
    setenv("FAKE_DROP_DURING_ASSETS","1",1);
    CHECK(has(sync(),"SYNC\t1\t0\t0\t2"));
    CHECK(catalog_requests()==std::vector<std::string>{github}); selected("auto","default");
    unsetenv("FAKE_DROP_DURING_ASSETS"); fs::remove(root/"cache/network-drop-marker");
    unsetenv("FAKE_CN_REGISTRY");
    CHECK(call({"--set-region","default"}).rc==0); CHECK(has(sync(),"SYNC\t1\t0\t0\t2"));
}
#ifdef APPSTORE_NATIVE_IN_PROCESS_TEST
static void wait_for_request(const std::string &needle) {
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(2);
    while (std::chrono::steady_clock::now()<deadline) {
        std::ifstream input(root/"cache/network.log");
        std::string line;
        while (std::getline(input,line)) if (line.find(needle)!=std::string::npos) return;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    CHECK(false);
}
static void concurrent_sync_cases() {
    for (int change=0; change<3; ++change) {
        CHECK(call({"--set-region","auto"}).rc==0);
        text(root/"cache/network.log", "");
        setenv("FAKE_DEFAULT_OUTCOME","slow",1);
        Result result;
        std::thread sync([&]() { result.out=appstore::native_backend_capture({"--sync"}, &result.rc); });
        wait_for_request("github.io");
        Result busy;
        busy.out=appstore::native_backend_capture({"--sync"}, &busy.rc);
        CHECK(busy.rc!=0 && has(busy,"sync already running"));
        CHECK(call({"--sync"}).rc!=0);
        if (change==0) {
            int rc=1;
            appstore::native_backend_capture({"--set-region","CN"}, &rc); CHECK(rc==0);
        } else if (change==1) {
            CHECK(call({"--set-region","CN"}).rc==0);
        } else {
            CHECK(call({"--set-region","CN"}).rc==0);
            CHECK(call({"--set-region","auto"}).rc==0);
            CHECK(call({"--replace-registry-config", "{\"region\":\"auto\",\"active_region\":\"default\"}"}).rc==0);
        }
        sync.join();
        CHECK(result.rc!=0 && has(result,"superseded"));
        CHECK(has(call({"--registry-config"}),change==2 ? "CONFIG\tauto\tdefault\t1" : "CONFIG\tCN\tCN\t1"));
        unsetenv("FAKE_DEFAULT_OUTCOME");
    }
    CHECK(call({"--clear-registry-cache"}).rc==0);
    CHECK(call({"--set-region","CN"}).rc==0);
    CHECK(call({"--sync"}).rc==0);
    CHECK(call({"--set-region","auto"}).rc==0);
    fs::remove_all(root/"cache/icons"); fs::create_directories(root/"cache/icons");
    text(root/"cache/network.log", ""); setenv("FAKE_ASSET_SLEEP","4",1);
    Result result;
    std::thread sync([&]() { result.out=appstore::native_backend_capture({"--sync"}, &result.rc); });
    wait_for_request(".png");
    CHECK(has(call({"--registry-config"}),"CONFIG\tauto\tCN\t1"));
    appstore::native_request_sync_cancel();
    int busy_rc=0;
    appstore::native_backend_capture({"--sync"},&busy_rc); CHECK(busy_rc!=0);
    sync.join();
    CHECK(result.rc!=0 && has(result,"Sync cancelled"));
    CHECK(has(call({"--registry-config"}),"CONFIG\tauto\tCN\t1"));
    CHECK(has(call({"--summary"}),"APP\tapp-id"));
    CHECK(has(call({"--registries"}),"aliyuncs.com/packages/cn/registry.json\tcached\t2"));
    unsetenv("FAKE_ASSET_SLEEP");
    CHECK(call({"--sync"}).rc==0);
    CHECK(has(call({"--registry-config"}),"CONFIG\tauto\tdefault\t1"));
}
static void optional_metadata_cases() {
    const json index=load(root/"registry.json");
    for (const json &bad : {json{{"apps", {{{"uuid",42}}}}}, json{{"apps",42}},
                           json{{"apps", {{{"uuid","extra"},{"featured",42}},{{"uuid","other"}}}}}}) {
        save(root/"registry.json",bad);
        json record=json::parse(appstore::native_test_sync_registry("https://metadata.invalid/catalog", index.dump()));
        CHECK(record["status"]=="ok" && record["index"]==index && record["full"]==json::object());
        // Feed the persisted result through the normal summary/merge path.
        CHECK(call({"--set-region","default"}).rc==0);
        for (const auto &entry : fs::directory_iterator(root/"cache")) {
            if (entry.path().filename().string().rfind("registry-",0)!=0) continue;
            json cached=load(entry.path());
            if (cached.value("url","")=="https://cardputerzero.github.io/generated/registry.json") {
                record["url"]=cached["url"]; save(entry.path(),record); break;
            }
        }
        Result summary=call({"--summary"}); CHECK(summary.rc==0 && has(summary,"APP\tapp-id"));
    }
    save(root/"registry.json",index);
    CHECK(call({"--sync"}).rc==0);
}
static void cancellation_cases() {
    CHECK(call({"--set-region","auto"}).rc==0);
    text(root/"cache/network.log", "");
    setenv("FAKE_DEFAULT_OUTCOME","slow",1);
    Result result;
    std::thread sync([&]() { result.out=appstore::native_backend_capture({"--sync"}, &result.rc); });
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(2);
    while (fs::file_size(root/"cache/network.log")==0 && std::chrono::steady_clock::now()<deadline)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    CHECK(fs::file_size(root/"cache/network.log")>0);
    const auto cancelled=std::chrono::steady_clock::now();
    appstore::native_request_sync_cancel();
    sync.join();
    CHECK(std::chrono::steady_clock::now()-cancelled < std::chrono::seconds(2));
    CHECK(result.rc!=0 && has(result,"Sync cancelled"));
    CHECK(catalog_requests().size()==1);
    CHECK(appstore::native_backend_capture({"--sync-status"},nullptr).find("cancelled")!=std::string::npos);
    unsetenv("FAKE_DEFAULT_OUTCOME");
    int rc=1;
    CHECK(appstore::native_backend_capture({"--sync"},&rc).find("SYNC\t1\t0\t0\t2")!=std::string::npos && rc==0);
    CHECK(call({"--set-region","default"}).rc==0);
}
#endif
static void plan_cases() {
    Result p=call({"--plan","demo"}); CHECK(p.rc==0&&has(p,"PLAN\tapp-id\tDemo Local\t1.2.0\t42K")); CHECK(has(p,"lib-one,lib-two"));
    CHECK(call({"--plan","blocked"}).rc!=0); CHECK(call({"--plan","missing"}).rc!=0);
}
static void complete(const fs::path &p) { json v=load(p);v["helper_completed"]=true;save(p,v); }
static json pending_job(const std::string &id, bool previously_installed=false, const std::string &previous_version="") {
    return {{"schema_version",2},{"transaction_id",id},{"action","install"},{"app_id","app-id"},
      {"package","demo-package"},{"previously_installed",previously_installed},{"previous_version",previous_version},
      {"expected_package_version","1.2.0"},{"deb_path",(root/"cache/downloads/cached-demo.deb").string()},
      {"helper_completed",false},{"app_snapshot",app("app-id","approved")}};
}
static void package_cases() {
    fs::path pending=root/"state/pending-package.json", installed=root/"state/installed.json";
    setenv("FAKE_DPKG_MODE","absent",1); Result prep=call({"--prepare-package","install","demo"}); std::string id=tx(prep);
    CHECK(prep.rc==0&&!id.empty()&&fs::exists(pending)); json j=load(pending); CHECK(j["previously_installed"]==false&&j["expected_package_version"]=="1.2.0");
    CHECK(call({"--repair-package-transaction","blocked-id"}).rc!=0&&fs::exists(pending));
    Result repaired=call({"--repair-package-transaction","app-id"});
    CHECK(repaired.rc==0&&has(repaired,"PACKAGE_REPAIRED\tapp-id")&&!fs::exists(pending));
    fs::create_directories(root/"cache/downloads");
    fs::copy_file(root/"demo.deb", root/"cache/downloads/cached-demo.deb",
                  fs::copy_options::overwrite_existing);
    save(pending, pending_job("conflict"));
    setenv("FAKE_DPKG_MODE","installed",1); setenv("FAKE_VERSION","9.9.0",1);
    Result conflict=call({"--prepare-package","install","blocked"});
    CHECK(conflict.rc!=0&&has(conflict,"PENDING_CONFLICT\tapp-id\tinstall\tdemo-package"));
    repaired=call({"--repair-package-transaction","app-id"});
    CHECK(repaired.rc==0&&has(repaired,"PACKAGE_REPAIR_READY\tapp-id\tdemo-package")&&fs::exists(pending));
    CHECK(!load(pending).value("helper_completed",true));
    Result recovered_retry=call({"--prepare-package","install","demo"});
    CHECK(recovered_retry.rc==0&&tx(recovered_retry)=="conflict"&&
          has(recovered_retry,"PACKAGE_JOB\trepair\tdemo-package"));
    fs::remove(pending); unsetenv("FAKE_VERSION"); setenv("FAKE_DPKG_MODE","absent",1);
    prep=call({"--prepare-package","install","demo"}); id=tx(prep);
    CHECK(prep.rc==0&&!id.empty()&&fs::exists(pending));
    Result retry=call({"--prepare-package","install","demo"}); CHECK(retry.rc==0&&tx(retry)==id);
    CHECK(call({"--finalize-package","install","demo","wrong"}).rc!=0);
    CHECK(call({"--finalize-package","install","demo",id}).rc!=0&&fs::exists(pending)); complete(pending);
    setenv("FAKE_DPKG_MODE","installed",1); Result done=call({"--finalize-package","install","demo",id}); CHECK(done.rc==0&&has(done,"PACKAGE_RESULT\tinstall\tapp-id\tdemo-package\t1.2.0"));
    CHECK(!fs::exists(pending)&&load(installed)["app-id"]["files"].size()==2); CHECK(call({"--finalize-package","install","demo",id}).rc==0);
    Result un=call({"--prepare-package","uninstall","demo"}); std::string uid=tx(un); CHECK(un.rc==0&&!uid.empty()); complete(pending);setenv("FAKE_DPKG_MODE","absent",1);
    CHECK(call({"--finalize-package","uninstall","demo",uid}).rc==0); CHECK(!load(installed).contains("app-id")); CHECK(call({"--finalize-package","uninstall","demo",uid}).rc==0);
    fs::copy_file(root/"demo.deb",root/"cache/downloads/cached-demo.deb",fs::copy_options::overwrite_existing);
    save(pending,pending_job("idle"));
    CHECK(call({"--summary"}).rc==0&&fs::exists(pending));
    CHECK(tx(call({"--prepare-package","install","demo"}))=="idle"); fs::remove(pending);
    save(pending,pending_job("unknown-query")); setenv("FAKE_DPKG_MODE","error",1);
    CHECK(call({"--summary"}).rc==0&&fs::exists(pending)); fs::remove(pending);
    save(pending,pending_job("un-status")); setenv("FAKE_DPKG_MODE","un",1);
    CHECK(call({"--summary"}).rc==0&&fs::exists(pending)); fs::remove(pending);
    save(pending,pending_job("rc-status")); setenv("FAKE_DPKG_MODE","installed",1); setenv("FAKE_STATUS","rc ",1);
    CHECK(call({"--summary"}).rc==0&&fs::exists(pending)); fs::remove(pending);
    save(pending,pending_job("ri-status",true,"1.2.0")); setenv("FAKE_STATUS","ri ",1);
    CHECK(call({"--summary"}).rc==0&&fs::exists(pending)); fs::remove(pending);
    save(pending,pending_job("unknown-status")); setenv("FAKE_STATUS","xx ",1);
    CHECK(call({"--summary"}).rc==0&&fs::exists(pending)); fs::remove(pending); unsetenv("FAKE_STATUS");
    save(pending,{{"schema_version",2},{"transaction_id","applied"},{"action","install"},{"app_id","app-id"},{"package","demo-package"},{"previously_installed",false},{"previous_version",""},{"expected_package_version","1.2.0"},{"helper_completed",false},{"app_snapshot",app("app-id","approved")}});
    setenv("FAKE_DPKG_MODE","installed",1); CHECK(call({"--summary"}).rc==0&&!fs::exists(pending)&&load(installed).contains("app-id"));
    save(pending,{{"schema_version",2},{"transaction_id","applied-uninstall"},{"action","uninstall"},{"app_id","app-id"},{"package","demo-package"},{"previously_installed",true},{"previous_version","1.2.0"},{"helper_completed",false},{"app_snapshot",app("app-id","approved")}});
    setenv("FAKE_DPKG_MODE","absent",1); CHECK(call({"--summary"}).rc==0&&!fs::exists(pending)&&!load(installed).contains("app-id"));
    save(pending,{{"schema_version",2},{"transaction_id","partial-uninstall"},{"action","uninstall"},{"app_id","app-id"},{"package","demo-package"},{"previously_installed",true},{"previous_version","1.2.0"},{"helper_completed",false},{"app_snapshot",app("app-id","approved")}});
    setenv("FAKE_DPKG_MODE","error",1); Result partial_summary=call({"--summary"});
    CHECK(partial_summary.rc==0&&has(partial_summary,"APP\tapp-id\tDemo Local\t1.2.0\tUtilities\t1"));
    CHECK(has(partial_summary,"WARN\tInterrupted uninstall pending"));
    Result retry_uninstall=call({"--prepare-package","uninstall","demo"});
    CHECK(retry_uninstall.rc==0&&tx(retry_uninstall)=="partial-uninstall"); fs::remove(pending);
    text(pending,"{broken json\n"); CHECK(call({"--summary"}).rc==0&&!fs::exists(pending));
    setenv("FAKE_DPKG_MODE","installed",1); Result reinstall=call({"--prepare-package","reinstall","demo"});
    CHECK(reinstall.rc==0&&!tx(reinstall).empty()&&has(reinstall,"PACKAGE_JOB\tinstall")&&has(reinstall,"\t1\t"));
    Result reinstall_retry=call({"--prepare-package","reinstall","demo"}); CHECK(tx(reinstall_retry)==tx(reinstall)); fs::remove(pending);
    setenv("FAKE_DPKG_MODE","absent",1);
    CHECK(call({"--prepare-package","uninstall","demo"}).rc!=0);
    CHECK(call({"--prepare-package","reinstall","demo"}).rc!=0);
    CHECK(call({"--prepare-package","upgrade","demo"}).rc!=0);
    setenv("FAKE_DPKG_MODE","installed",1);
    CHECK(call({"--prepare-package","install","demo"}).rc!=0);
    save(pending,{{"schema_version",2},{"transaction_id","badver"},{"action","upgrade"},{"app_id","app-id"},{"package","demo-package"},{"previously_installed",true},{"previous_version","1.1.0"},{"expected_package_version","1.2.0"},{"helper_completed",true},{"app_snapshot",app("app-id","approved")}});
    setenv("FAKE_VERSION","1.1.0",1); CHECK(call({"--finalize-package","upgrade","demo","badver"}).rc!=0&&fs::exists(pending));
    CHECK(call({"--summary"}).rc==0&&!load(pending).value("helper_completed",true));
    CHECK(load(pending).value("recovery_reason","")=="package state changed before finalization");
    fs::remove(pending); unsetenv("FAKE_VERSION");
    save(pending,pending_job("files-fail")); complete(pending); setenv("FAKE_FILES_FAIL","1",1);
    CHECK(call({"--finalize-package","install","demo","files-fail"}).rc!=0&&fs::exists(pending)); fs::remove(pending); unsetenv("FAKE_FILES_FAIL");
    setenv("FAKE_DEB_PACKAGE","wrong",1); fs::remove_all(root/"cache/downloads"); CHECK(call({"--prepare-package","install","demo"}).rc!=0&&!fs::exists(pending)); unsetenv("FAKE_DEB_PACKAGE");
    fs::remove_all(root/"cache/downloads"); setenv("FAKE_CURL_SLEEP","3",1);
    setenv("FAKE_DPKG_MODE","absent",1); call_and_kill({"--prepare-package","install","demo"});
    unsetenv("FAKE_CURL_SLEEP"); CHECK(!fs::exists(pending));
    Result after_kill=call({"--prepare-package","install","demo"}); CHECK(after_kill.rc==0&&!tx(after_kill).empty());
    int ready[2]; CHECK(pipe(ready)==0); pid_t holder=fork(); CHECK(holder>=0);
    if (!holder) { close(ready[0]); int lockfd=open((root/"state/package-transaction.lock").c_str(),O_RDWR|O_CREAT,0644); if(lockfd<0||flock(lockfd,LOCK_EX)!=0)_exit(2); write(ready[1],"x",1); sleep(5); _exit(0); }
    close(ready[1]); char marker=0; CHECK(read(ready[0],&marker,1)==1); close(ready[0]);
    setenv("M5APPSTORE_LOCK_TIMEOUT","1",1); Result busy=call({"--prepare-package","install","demo"});
    CHECK(busy.rc!=0&&has(busy,"package transaction is still busy")); unsetenv("M5APPSTORE_LOCK_TIMEOUT");
    kill(holder,SIGTERM); int holder_status=0; CHECK(waitpid(holder,&holder_status,0)==holder); fs::remove(pending);
    CHECK(call({"--install","demo"}).rc!=0); CHECK(has(call({"--package-helper","install"}),"package helper requires root"));
}
int main(int argc,char **argv) {
    CHECK(argc==2); binary=fs::absolute(argv[1]); char t[]="/tmp/appstore-cpp-parity.XXXXXX"; char *p=mkdtemp(t);CHECK(p);root=p;
    fixture();registry_cases();source_selection_cases();
#ifdef APPSTORE_NATIVE_IN_PROCESS_TEST
    concurrent_sync_cases();
    optional_metadata_cases();
    cancellation_cases();
#endif
    plan_cases();package_cases();fs::remove_all(root);std::cout<<"native C++ parity tests passed: "<<checks<<" checks\n";
}
