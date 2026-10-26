#include "vcm/can_socket.hpp"

#include <linux/can/raw.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>

CanSocket::~CanSocket() {
    close();
}

bool CanSocket::open(const std::string& ifname) {
    close();

    sock_ = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (sock_ < 0) {
        perror("socket");
        return false;
    }

    struct ifreq ifr;
    std::memset(&ifr, 0, sizeof(ifr));
    std::strncpy(ifr.ifr_name, ifname.c_str(), IFNAMSIZ - 1);

    if (ioctl(sock_, SIOCGIFINDEX, &ifr) < 0) {
        perror("ioctl SIOCGIFINDEX");
        close();
        return false;
    }

    struct sockaddr_can addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(sock_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close();
        return false;
    }

    // 默认超时 100ms，receive 会覆盖
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 100 * 1000;
    setsockopt(sock_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    return true;
}

void CanSocket::close() {
    if (sock_ >= 0) {
        ::close(sock_);
        sock_ = -1;
    }
}

bool CanSocket::send(const CanFrame& frame) {
    if (sock_ < 0) return false;

    struct can_frame raw = frame.toRaw();
    ssize_t n = write(sock_, &raw, sizeof(raw));
    if (n != sizeof(raw)) {
        perror("write");
        return false;
    }
    return true;
}

bool CanSocket::receive(CanFrame& frame, int timeout_ms) {
    if (sock_ < 0) return false;

    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    setsockopt(sock_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    struct can_frame raw;
    ssize_t n = read(sock_, &raw, sizeof(raw));
    if (n < 0) {
        return false;   // 超时或错误
    }
    if (n != sizeof(raw)) {
        return false;
    }

    frame = CanFrame::fromRaw(raw);
    return true;
}

bool CanSocket::setFilter(uint32_t filter_id, uint32_t mask) {
    std::vector<struct can_filter> f = { { filter_id, mask } };
    return setFilters(f);
}

bool CanSocket::setFilters(const std::vector<struct can_filter>& filters) {
    if (sock_ < 0) return false;

    if (filters.empty()) {
        return clearFilter();
    }

    if (setsockopt(sock_, SOL_CAN_RAW, CAN_RAW_FILTER,
                   filters.data(),
                   static_cast<socklen_t>(filters.size() * sizeof(struct can_filter))) < 0) {
        perror("setsockopt CAN_RAW_FILTER");
        return false;
    }
    return true;
}

bool CanSocket::clearFilter() {
    if (sock_ < 0) return false;

    if (setsockopt(sock_, SOL_CAN_RAW, CAN_RAW_FILTER, nullptr, 0) < 0) {
        perror("setsockopt CAN_RAW_FILTER clear");
        return false;
    }
    return true;
}

bool CanSocket::loadFiltersFromFile(const std::string& path) {
    if (sock_ < 0) return false;

    std::ifstream ifs(path);
    if (!ifs.is_open()) {
        std::cerr << "打开过滤器文件失败: " << path << "\n";
        return false;
    }

    std::vector<struct can_filter> filters;
    std::string line;
    int line_no = 0;

    while (std::getline(ifs, line)) {
        ++line_no;

        // 去掉注释部分
        auto pos = line.find('#');
        if (pos != std::string::npos) {
            line = line.substr(0, pos);
        }

        std::istringstream iss(line);
        std::string id_str, mask_str;
        if (!(iss >> id_str)) continue;         // 空行
        if (!(iss >> mask_str)) {
            std::cerr << "第 " << line_no << " 行缺少 mask，跳过\n";
            continue;
        }

        try {
            uint32_t id   = static_cast<uint32_t>(std::stoul(id_str, nullptr, 0));
            uint32_t mask = static_cast<uint32_t>(std::stoul(mask_str, nullptr, 0));

            struct can_filter f;
            f.can_id = id;
            f.can_mask = mask;
            filters.push_back(f);
        } catch (const std::exception& e) {
            std::cerr << "第 " << line_no << " 行解析失败: " << e.what() << "\n";
        }
    }

    if (filters.empty()) {
        std::cerr << "过滤器文件为空: " << path << "\n";
        return false;
    }

    std::cout << "从文件加载了 " << filters.size() << " 条过滤规则\n";
    return setFilters(filters);
}