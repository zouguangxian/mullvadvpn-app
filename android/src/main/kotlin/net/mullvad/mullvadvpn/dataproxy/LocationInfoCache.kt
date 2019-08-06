package net.mullvad.mullvadvpn.dataproxy

import kotlinx.coroutines.launch
import kotlinx.coroutines.Deferred
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.GlobalScope
import kotlinx.coroutines.Job

import net.mullvad.mullvadvpn.model.ActionAfterDisconnect
import net.mullvad.mullvadvpn.model.GeoIpLocation
import net.mullvad.mullvadvpn.model.TunnelState
import net.mullvad.mullvadvpn.MullvadDaemon
import net.mullvad.mullvadvpn.relaylist.RelayCity
import net.mullvad.mullvadvpn.relaylist.RelayCountry
import net.mullvad.mullvadvpn.relaylist.Relay

class LocationInfoCache(
    val daemon: Deferred<MullvadDaemon>,
    val relayListListener: RelayListListener
) {
    private var lastKnownRealLocation: GeoIpLocation? = null

    var onNewLocation: ((GeoIpLocation?) -> Unit)? = null
        set(value) {
            field = value
            value?.invoke(location)
        }

    var location: GeoIpLocation? = null
        set(value) {
            field = value
            onNewLocation?.invoke(value)
        }

    var state: TunnelState = TunnelState.Disconnected()
        set(value) {
            if (field != value) {
                field = value

                when (value) {
                    is TunnelState.Disconnected -> {
                        location = lastKnownRealLocation
                        fetchLocation()
                    }
                    is TunnelState.Connecting -> location = value.location
                    is TunnelState.Connected -> {
                        location = value.location
                        fetchLocation()
                    }
                    is TunnelState.Disconnecting -> {
                        location = when (value.actionAfterDisconnect) {
                            is ActionAfterDisconnect.Nothing -> lastKnownRealLocation
                            is ActionAfterDisconnect.Block -> null
                            is ActionAfterDisconnect.Reconnect -> locationFromSelectedRelay()
                        }
                    }
                    is TunnelState.Blocked -> location = null
                }
            }
        }

    private fun locationFromSelectedRelay(): GeoIpLocation? {
        val relayItem = relayListListener.selectedRelayItem

        when (relayItem) {
            is RelayCountry -> return GeoIpLocation(null, null, relayItem.name, null, null)
            is RelayCity -> return GeoIpLocation(
                null,
                null,
                relayItem.country.name,
                relayItem.name,
                null
            )
            is Relay -> return GeoIpLocation(
                null,
                null,
                relayItem.city.country.name,
                relayItem.city.name,
                relayItem.name
            )
        }

        return null
    }

    private fun fetchLocation() {
        stateToFetchFor = state

        GlobalScope.launch(Dispatchers.Default) {
            requestToFetchLocation()
        }
    }

    private fun shouldRetryFetch(): Boolean {
        val state = this.state

        return state is TunnelState.Disconnected ||
            state is TunnelState.Connected
    }

    private fun newLocationFetched(newLocation: GeoIpLocation?) {
        if (newLocation == null) {
            if (shouldRetryFetch() && state == stateToFetchFor) {
                requestToFetchLocation()
            }
        } else if (state == stateToFetchFor) {
            GlobalScope.launch(Dispatchers.Main) {
                when (state) {
                    is TunnelState.Disconnected -> {
                        lastKnownRealLocation = newLocation
                        location = newLocation
                    }
                    is TunnelState.Connecting -> location = newLocation
                    is TunnelState.Connected -> location = newLocation
                }
            }
        }
    }

    private external fun requestToFetchLocation()
}
