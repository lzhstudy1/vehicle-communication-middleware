#include"vcm/DTC_Manager.hpp"

void DtcManager::reportDtc(uint32_t code, uint8_t mask){
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = std::find_if(dtc_list_.begin(),dtc_list_.end(),
                        [code](const DtcRecord& r){return r.code == code;});
    if(it == dtc_list_.end()){
        dtc_list_.push_back({code,mask,40});
    }else{
        it->status |= mask;
    }
}

void DtcManager::clearAllDtc(){
    std::lock_guard<std::mutex> lock(mutex_);
    dtc_list_.clear();
}

void DtcManager::clearDtcStatus(uint32_t code,uint8_t mask){
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = std::find_if(dtc_list_.begin(),dtc_list_.end(),
                        [code](const DtcRecord& r){return r.code == code;});
    if(it != dtc_list_.end()){
        it->status &= ~mask;
    }
}

bool DtcManager::hasDtc(uint32_t code) const{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = std::find_if(dtc_list_.begin(),dtc_list_.end(),
                        [code](const DtcRecord& r){return r.code == code;});
    return it != dtc_list_.end() ? true : false;
}

std::vector<DtcRecord> DtcManager::getDtcList(uint8_t mask) const{
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<DtcRecord> result;
    for(auto dtc : dtc_list_){
        if(dtc.status & mask) result.push_back(dtc);
    }
    return result;
}

size_t DtcManager::getDtcCount(uint8_t mask) const{
    std::lock_guard<std::mutex> lock(mutex_);
    size_t count = 0;
    for(auto dtc : dtc_list_){
        if(dtc.status & mask) ++count;
    }
    return count;
}

void DtcManager::processAgingCycle() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (auto& dtc : dtc_list_) {
        if ((dtc.status & static_cast<uint8_t>(DtcStatus::TEST_FAILED)) != 0) {
            dtc.aging_counter = 40;
            continue;
        }

        if ((dtc.status & static_cast<uint8_t>(DtcStatus::CONFIRMED)) != 0) {
            if (dtc.aging_counter > 0) {
                dtc.aging_counter--;
            }

            if (dtc.aging_counter == 0) {
                dtc.status &= ~static_cast<uint8_t>(DtcStatus::CONFIRMED);
                dtc.status &= ~static_cast<uint8_t>(DtcStatus::WARNING_LIGHT_REQUESTED);
            }
        }
    }

    auto it = std::remove_if(dtc_list_.begin(), dtc_list_.end(), 
        [](const DtcRecord& r) {
            return r.status == 0; 
        });
    dtc_list_.erase(it, dtc_list_.end());
}