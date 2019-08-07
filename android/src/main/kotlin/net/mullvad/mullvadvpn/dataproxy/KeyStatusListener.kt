package net.mullvad.mullvadvpn.dataproxy

import kotlinx.coroutines.launch
import kotlinx.coroutines.Deferred
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.GlobalScope
import kotlinx.coroutines.Job

import net.mullvad.mullvadvpn.MullvadDaemon
import net.mullvad.mullvadvpn.model.KeygenEvent

class KeyStatusListener(val asyncDaemon: Deferred<MullvadDaemon>) {
    private var daemon: MullvadDaemon? = null
    private var retryJob: Job? = null

    private val setUpJob = setUp()

    var keyStatus: KeygenEvent? = null
        private set(value) {
            synchronized(this) {
                field = value

                if (value != null) {
                    onKeyStatusChange?.invoke(value)
                }
            }
        }

    var onKeyStatusChange: ((KeygenEvent) -> Unit)? = null
        set(value) {
            field = value

            synchronized(this) {
                keyStatus?.let { status -> value?.invoke(status) }
            }
        }

    private fun setUp() = GlobalScope.launch(Dispatchers.Default) {
        daemon = asyncDaemon.await()
        daemon?.onKeygenEvent = { event -> keyStatus = event }
        val wireguardKey = daemon?.getWireguardKey()
        if (wireguardKey != null) {
            keyStatus = KeygenEvent.NewKey(wireguardKey, null)
        }
    }

    fun generateKey() = GlobalScope.launch(Dispatchers.Default) {
            setUpJob.join()
            keyStatus = daemon?.generateWireguardKey()
    }

    fun verifyKey() = GlobalScope.launch(Dispatchers.Default) {
            setUpJob.join()
            val verified = daemon?.verifyWireguardKey()
            // Only update verification status if the key is actually there
            when (val state = keyStatus) {
                is KeygenEvent.NewKey -> {
                    keyStatus = KeygenEvent.NewKey(state.publicKey, verified)
                }
            }
    }

    fun onDestroy() {
        setUpJob.cancel()
        retryJob?.cancel()
        daemon?.onKeygenEvent = null
    }

    private fun retryKeyGeneration() = GlobalScope.launch(Dispatchers.Default) {
        setUpJob.join()
        keyStatus = daemon?.generateWireguardKey()
    }
}
