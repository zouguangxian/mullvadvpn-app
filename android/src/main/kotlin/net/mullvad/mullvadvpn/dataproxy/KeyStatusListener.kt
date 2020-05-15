package net.mullvad.mullvadvpn.dataproxy

import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.GlobalScope
import kotlinx.coroutines.launch
import net.mullvad.mullvadvpn.model.KeygenEvent
import net.mullvad.mullvadvpn.service.MullvadDaemon

class KeyStatusListener(val daemon: MullvadDaemon) {
    private val setUpJob = setUp()

    var keyStatus: KeygenEvent? = null
        private set(value) {
            synchronized(this) {
                when (value) {
                    null -> android.util.Log.d("mullvad", "keyStatus = null")
                    is KeygenEvent.NewKey -> {
                        val verified = when (value.verified) {
                            null -> "null"
                            true -> "verified"
                            false -> "not verified"
                        }

                        val publicKey = value.publicKey
                        val failure = value.replacementFailure

                        android.util.Log.d("mullvad", "keyStatus = NewKey($publicKey, $verified, $failure)")
                    }
                    is KeygenEvent.TooManyKeys -> {
                        android.util.Log.d("mullvad", "keyStatus = TooManyKeys")
                    }
                    is KeygenEvent.GenerationFailure -> {
                        android.util.Log.d("mullvad", "keyStatus = GenerationFailure")
                    }
                }
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
        daemon.onKeygenEvent = { event -> keyStatus = event }
        val wireguardKey = daemon.getWireguardKey()
        if (wireguardKey != null) {
            keyStatus = KeygenEvent.NewKey(wireguardKey, null, null)
        }
    }

    fun generateKey() = GlobalScope.launch(Dispatchers.Default) {
        android.util.Log.d("mullvad", "KeyStatusListener.generateKey()")
        setUpJob.join()
        val oldStatus = keyStatus
        android.util.Log.d("mullvad", "  generating key")
        val newStatus = daemon.generateWireguardKey()
        android.util.Log.d("mullvad", "  key generation finished")
        val newFailure = newStatus?.failure()
        if (oldStatus is KeygenEvent.NewKey && newFailure != null) {
            android.util.Log.d("mullvad", "  key generation failed")
            keyStatus = KeygenEvent.NewKey(oldStatus.publicKey,
                            oldStatus.verified,
                            newFailure)
        } else {
            android.util.Log.d("mullvad", "  key generation complete")
            keyStatus = newStatus ?: KeygenEvent.GenerationFailure()
        }
    }

    fun verifyKey() = GlobalScope.launch(Dispatchers.Default) {
        setUpJob.join()
        val verified = daemon.verifyWireguardKey()
        // Only update verification status if the key is actually there
        when (val state = keyStatus) {
            is KeygenEvent.NewKey -> {
                keyStatus = KeygenEvent.NewKey(state.publicKey,
                                verified,
                                state.replacementFailure)
            }
        }
    }

    fun onDestroy() {
        setUpJob.cancel()
        daemon.onKeygenEvent = null
    }

    private fun retryKeyGeneration() = GlobalScope.launch(Dispatchers.Default) {
        setUpJob.join()
        keyStatus = daemon.generateWireguardKey()
    }
}
