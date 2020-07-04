package net.mullvad.mullvadvpn.service

import android.content.Context
import java.io.File
import kotlin.properties.Delegates.observable
import net.mullvad.mullvadvpn.model.TunnelState
import net.mullvad.talpid.tunnel.ActionAfterDisconnect

private const val SHARED_PREFERENCES = "split_tunnelling"
private const val KEY_ENABLED = "enabled"

class SplitTunnelling(private val context: Context, private val connectionProxy: ConnectionProxy) {
    private val appListFile = File(context.filesDir, "split-tunnelling.txt")
    private val excludedApps = HashSet<String>()
    private val preferences = context.getSharedPreferences(SHARED_PREFERENCES, Context.MODE_PRIVATE)

    val excludedAppList
        get() = if (enabled) {
            excludedApps.toList()
        } else {
            emptyList()
        }

    var enabled by observable(preferences.getBoolean(KEY_ENABLED, false)) { _, _, _ ->
        enabledChanged()
    }

    var onChange: ((List<String>) -> Unit)? = null

    init {
        if (appListFile.exists()) {
            excludedApps.addAll(appListFile.readLines())
        }
    }

    fun isAppExcluded(appPackageName: String) = excludedApps.contains(appPackageName)

    fun excludeApp(appPackageName: String) {
        excludedApps.add(appPackageName)
        update()
    }

    fun includeApp(appPackageName: String) {
        excludedApps.remove(appPackageName)
        update()
    }

    fun persist() {
        appListFile.writeText(excludedApps.joinToString(separator = "\n"))
    }

    private fun enabledChanged() {
        preferences.edit().apply {
            putBoolean(KEY_ENABLED, enabled)
            commit()
        }

        update()
    }

    private fun update() {
        onChange?.invoke(excludedAppList)

        val state = connectionProxy.uiState

        val shouldReconnect = when (state) {
            is TunnelState.Disconnected -> false
            is TunnelState.Connecting -> true
            is TunnelState.Connected -> true
            is TunnelState.Disconnecting -> {
                state.actionAfterDisconnect != ActionAfterDisconnect.Nothing
            }
            is TunnelState.Error -> state.errorState.isBlocking
        }

        if (shouldReconnect) {
            connectionProxy.replaceTun()
        }
    }
}
