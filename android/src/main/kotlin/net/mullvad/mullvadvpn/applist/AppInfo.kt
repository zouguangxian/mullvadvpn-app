package net.mullvad.mullvadvpn.applist

import android.content.pm.ApplicationInfo
import android.content.pm.PackageManager

data class AppInfo(val info: ApplicationInfo, val label: String)
