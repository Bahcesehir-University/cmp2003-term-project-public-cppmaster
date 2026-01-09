#include "analyzer.h"

#include <fstream>
#include <algorithm>
#include <cctype>     



static inline bool isDigit(char c) {
    return std::isdigit(static_cast<unsigned char>(c)) != 0;
}


static bool parseHour(const std::string& dt, int& hourOut) {
   
    if (dt.size() < 16) return false;
    if (dt[10] != ' ') return false;
    if (dt[13] != ':') return false;

    char h1 = dt[11], h2 = dt[12];
    if (!isDigit(h1) || !isDigit(h2)) return false;

    int hour = (h1 - '0') * 10 + (h2 - '0');
    if (hour < 0 || hour > 23) return false;

    hourOut = hour;
    return true;
}


static bool split6(const std::string& line, std::string fields[6]) {
    int start = 0;
    int idx = 0;
    const int n = static_cast<int>(line.size());

    for (int i = 0; i <= n; ++i) {
        if (i == n || line[i] == ',') {
            if (idx >= 6) return false; 
            fields[idx++] = line.substr(start, i - start);
            start = i + 1;
        }
    }
    return idx == 6;
}



void TripAnalyzer::ingestFile(const std::string& csvPath) {
    
    zoneCounts.clear();
    zoneHourCounts.clear();

    std::ifstream in(csvPath);
    if (!in.is_open()) {
       
        return;
    }

    std::string line;

    
    if (!std::getline(in, line)) return;

    while (std::getline(in, line)) {
        if (line.empty()) continue;

        std::string f[6];
        if (!split6(line, f)) continue;            

        const std::string& pickupZone = f[1];
        const std::string& pickupDT   = f[3];

        if (pickupZone.empty()) continue;          
        if (pickupDT.empty()) continue;            

        int hour = -1;
        if (!parseHour(pickupDT, hour)) continue;  

        
        zoneCounts[pickupZone]++;

        auto& arr = zoneHourCounts[pickupZone];
        
        arr[hour] += 1;
    }
}

std::vector<ZoneCount> TripAnalyzer::topZones(int k) const {
    std::vector<ZoneCount> v;
    v.reserve(zoneCounts.size());

    for (const auto& it : zoneCounts) {
        v.push_back(ZoneCount{it.first, it.second});
    }

    std::sort(v.begin(), v.end(), [](const ZoneCount& a, const ZoneCount& b) {
        if (a.count != b.count) return a.count > b.count; 
        return a.zone < b.zone;                           
    });

    if (k < 0) k = 0;
    if (static_cast<size_t>(k) < v.size()) v.resize(static_cast<size_t>(k));
    return v;
}

std::vector<SlotCount> TripAnalyzer::topBusySlots(int k) const {
    std::vector<SlotCount> v;
    v.reserve(zoneHourCounts.size() * 2); 

    for (const auto& it : zoneHourCounts) {
        const std::string& zone = it.first;
        const auto& arr = it.second;
        for (int h = 0; h < 24; ++h) {
            long long c = arr[h];
            if (c > 0) v.push_back(SlotCount{zone, h, c});
        }
    }

    std::sort(v.begin(), v.end(), [](const SlotCount& a, const SlotCount& b) {
        if (a.count != b.count) return a.count > b.count; 
        if (a.zone != b.zone)   return a.zone < b.zone;  
        return a.hour < b.hour;                          
    });

    if (k < 0) k = 0;
    if (static_cast<size_t>(k) < v.size()) v.resize(static_cast<size_t>(k));
    return v;
}

