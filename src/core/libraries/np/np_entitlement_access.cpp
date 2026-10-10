// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstring>
#include <vector>

#include "common/logging/log.h"
#include "core/libraries/app_content/app_content_error.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "core/libraries/np/np_entitlement_access.h"

namespace Libraries::Np::NpEntitlementAccess {

using AppContent::OrbisAppContentAddcontDownloadStatus;
using AppContent::OrbisNpUnifiedEntitlementLabel;

static s32 ConvertAppContentError(s32 result) {
    switch (result) {
    case ORBIS_APP_CONTENT_ERROR_PARAMETER:
        return ORBIS_NP_ENTITLEMENT_ACCESS_ERROR_PARAMETER;
    case ORBIS_APP_CONTENT_ERROR_DRM_NO_ENTITLEMENT:
        return ORBIS_NP_ENTITLEMENT_ACCESS_ERROR_NO_ENTITLEMENT;
    default:
        return result;
    }
}

static OrbisNpEntitlementAccessPackageType PackageType(
    OrbisAppContentAddcontDownloadStatus status) {
    return status == OrbisAppContentAddcontDownloadStatus::NoExtraData
               ? OrbisNpEntitlementAccessPackageType::Ps4Al
               : OrbisNpEntitlementAccessPackageType::Ps4Ac;
}

static void FillInfo(OrbisNpEntitlementAccessAddcontEntitlementInfo* info,
                     const AppContent::OrbisAppContentAddcontInfo& addcont) {
    info->entitlement_label = addcont.entitlement_label;
    info->package_type = PackageType(addcont.status);
    info->download_status = addcont.status;
}

s32 PS4_SYSV_ABI
sceNpEntitlementAccessInitialize(const OrbisNpEntitlementAccessInitParam* init_param,
                                 OrbisNpEntitlementAccessBootParam* boot_param) {
    LOG_INFO(Lib_NpEntitlementAccess, "called");
    AppContent::LoadAddcontInfo();
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpEntitlementAccessGetAddcontEntitlementInfo(
    u32 service_label, const OrbisNpUnifiedEntitlementLabel* entitlement_label,
    OrbisNpEntitlementAccessAddcontEntitlementInfo* info) {
    LOG_INFO(Lib_NpEntitlementAccess, "called");
    if (info == nullptr || entitlement_label == nullptr) {
        return ORBIS_NP_ENTITLEMENT_ACCESS_ERROR_PARAMETER;
    }
    AppContent::LoadAddcontInfo();
    AppContent::OrbisAppContentAddcontInfo addcont{};
    if (const s32 result =
            AppContent::sceAppContentGetAddcontInfo(service_label, entitlement_label, &addcont);
        result != ORBIS_OK) {
        return ConvertAppContentError(result);
    }
    FillInfo(info, addcont);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpEntitlementAccessGetAddcontEntitlementInfoList(
    u32 service_label, OrbisNpEntitlementAccessAddcontEntitlementInfo* list, u32 list_num,
    u32* hit_num) {
    LOG_INFO(Lib_NpEntitlementAccess, "called, list_num = {}", list_num);
    AppContent::LoadAddcontInfo();
    if (list == nullptr || list_num == 0) {
        return ConvertAppContentError(
            AppContent::sceAppContentGetAddcontInfoList(service_label, nullptr, 0, hit_num));
    }
    std::vector<AppContent::OrbisAppContentAddcontInfo> addcont(list_num);
    u32 num_found = 0;
    if (const s32 result = AppContent::sceAppContentGetAddcontInfoList(
            service_label, addcont.data(), list_num, &num_found);
        result != ORBIS_OK) {
        return ConvertAppContentError(result);
    }
    for (u32 i = 0; i < num_found; ++i) {
        FillInfo(&list[i], addcont[i]);
    }
    if (hit_num != nullptr) {
        *hit_num = num_found;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpEntitlementAccessGetEntitlementKey(
    u32 service_label, const OrbisNpUnifiedEntitlementLabel* entitlement_label,
    OrbisNpEntitlementAccessEntitlementKey* key) {
    LOG_INFO(Lib_NpEntitlementAccess, "called");
    if (key == nullptr || entitlement_label == nullptr) {
        return ORBIS_NP_ENTITLEMENT_ACCESS_ERROR_PARAMETER;
    }
    AppContent::LoadAddcontInfo();
    AppContent::OrbisAppContentGetEntitlementKey addcont_key{};
    if (const s32 result = AppContent::sceAppContentGetEntitlementKey(
            service_label, entitlement_label, &addcont_key);
        result != ORBIS_OK) {
        return ConvertAppContentError(result);
    }
    static_assert(sizeof(key->data) == sizeof(addcont_key.data));
    std::memcpy(key->data, addcont_key.data, sizeof(key->data));
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpEntitlementAccessGetSkuFlag(u32* sku_flag) {
    LOG_INFO(Lib_NpEntitlementAccess, "called");
    if (sku_flag == nullptr) {
        return ORBIS_NP_ENTITLEMENT_ACCESS_ERROR_PARAMETER;
    }
    *sku_flag = ORBIS_NP_ENTITLEMENT_ACCESS_SKU_FLAG_FULL;
    return ORBIS_OK;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("jO8DM8oyego", "libSceNpEntitlementAccess", 1, "libSceNpEntitlementAccess",
                 sceNpEntitlementAccessInitialize);
    LIB_FUNCTION("xddD23+8TfQ", "libSceNpEntitlementAccess", 1, "libSceNpEntitlementAccess",
                 sceNpEntitlementAccessGetAddcontEntitlementInfo);
    LIB_FUNCTION("TFyU+KFBv54", "libSceNpEntitlementAccess", 1, "libSceNpEntitlementAccess",
                 sceNpEntitlementAccessGetAddcontEntitlementInfoList);
    LIB_FUNCTION("5LiMEPuW0DQ", "libSceNpEntitlementAccess", 1, "libSceNpEntitlementAccess",
                 sceNpEntitlementAccessGetEntitlementKey);
    LIB_FUNCTION("lPDO62PpJIA", "libSceNpEntitlementAccess", 1, "libSceNpEntitlementAccess",
                 sceNpEntitlementAccessGetSkuFlag);
}

} // namespace Libraries::Np::NpEntitlementAccess
