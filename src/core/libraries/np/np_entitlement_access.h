// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/app_content/app_content.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Np::NpEntitlementAccess {

constexpr s32 ORBIS_NP_ENTITLEMENT_ACCESS_ERROR_PARAMETER = 0x817D0002;
constexpr s32 ORBIS_NP_ENTITLEMENT_ACCESS_ERROR_NO_ENTITLEMENT = 0x817D0007;

constexpr u32 ORBIS_NP_ENTITLEMENT_ACCESS_SKU_FLAG_FULL = 3;

enum class OrbisNpEntitlementAccessPackageType : u32 {
    None = 0,
    Ps4Gd = 1,
    Ps4Ac = 2,
    Ps4Al = 3,
};

struct OrbisNpEntitlementAccessAddcontEntitlementInfo {
    AppContent::OrbisNpUnifiedEntitlementLabel entitlement_label;
    OrbisNpEntitlementAccessPackageType package_type;
    AppContent::OrbisAppContentAddcontDownloadStatus download_status;
};
static_assert(sizeof(OrbisNpEntitlementAccessAddcontEntitlementInfo) == 28);

struct OrbisNpEntitlementAccessEntitlementKey {
    char data[16];
};

struct OrbisNpEntitlementAccessInitParam {
    char reserved[32];
};

struct OrbisNpEntitlementAccessBootParam {
    char reserved[32];
};

s32 PS4_SYSV_ABI
sceNpEntitlementAccessInitialize(const OrbisNpEntitlementAccessInitParam* init_param,
                                 OrbisNpEntitlementAccessBootParam* boot_param);
s32 PS4_SYSV_ABI sceNpEntitlementAccessGetAddcontEntitlementInfo(
    u32 service_label, const AppContent::OrbisNpUnifiedEntitlementLabel* entitlement_label,
    OrbisNpEntitlementAccessAddcontEntitlementInfo* info);
s32 PS4_SYSV_ABI sceNpEntitlementAccessGetAddcontEntitlementInfoList(
    u32 service_label, OrbisNpEntitlementAccessAddcontEntitlementInfo* list, u32 list_num,
    u32* hit_num);
s32 PS4_SYSV_ABI sceNpEntitlementAccessGetEntitlementKey(
    u32 service_label, const AppContent::OrbisNpUnifiedEntitlementLabel* entitlement_label,
    OrbisNpEntitlementAccessEntitlementKey* key);
s32 PS4_SYSV_ABI sceNpEntitlementAccessGetSkuFlag(u32* sku_flag);

void RegisterLib(Core::Loader::SymbolsResolver* sym);

} // namespace Libraries::Np::NpEntitlementAccess
