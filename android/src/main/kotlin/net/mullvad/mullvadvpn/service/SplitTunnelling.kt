package net.mullvad.mullvadvpn.service

import kotlin.properties.Delegates.observable
import net.mullvad.mullvadvpn.model.TunnelState
import net.mullvad.talpid.tunnel.ActionAfterDisconnect

class SplitTunnelling(private val connectionProxy: ConnectionProxy) {
    private val excludedApps = HashSet<String>()

    val excludedAppList
        get() = if (enabled) {
            excludedApps.toList()
        } else {
            emptyList()
        }

    var enabled by observable(false) { _, _, _ -> update() }
    var onChange: ((List<String>) -> Unit)? = null

    fun isAppExcluded(appPackageName: String) = excludedApps.contains(appPackageName)

    fun excludeApp(appPackageName: String) {
        excludedApps.add(appPackageName)
        update()
    }

    fun includeApp(appPackageName: String) {
        excludedApps.remove(appPackageName)
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
