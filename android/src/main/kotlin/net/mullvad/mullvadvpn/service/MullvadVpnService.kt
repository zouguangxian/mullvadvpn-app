package net.mullvad.mullvadvpn.service

import android.content.Intent
import android.net.VpnService
import android.os.Binder
import android.os.IBinder
import java.io.File
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.GlobalScope
import kotlinx.coroutines.Job
import kotlinx.coroutines.launch
import net.mullvad.mullvadvpn.model.Settings
import net.mullvad.mullvadvpn.service.tunnelstate.TunnelStateUpdater
import net.mullvad.mullvadvpn.ui.MainActivity
import net.mullvad.talpid.TalpidVpnService
import net.mullvad.talpid.util.EventNotifier

private const val RELAYS_FILE = "relays.json"

class MullvadVpnService : TalpidVpnService() {
    private enum class PendingAction {
        Connect,
        Disconnect,
    }

    private val binder = LocalBinder()
    private val serviceNotifier = EventNotifier<ServiceInstance?>(null)

    private var isStopping = false
    private var loggedIn = false

    private var startDaemonJob: Job? = null

    private var instance: ServiceInstance? = null
        set(value) {
            if (field != value) {
                field?.onDestroy()
                field = value
                serviceNotifier.notify(value)
            }
        }

    private lateinit var notificationManager: ForegroundNotificationManager
    private lateinit var tunnelStateUpdater: TunnelStateUpdater

    private var pendingAction: PendingAction? = null
        set(value) {
            field = value

            instance?.connectionProxy?.let { activeConnectionProxy ->
                when (value) {
                    PendingAction.Connect -> activeConnectionProxy.connect()
                    PendingAction.Disconnect -> activeConnectionProxy.disconnect()
                    null -> {}
                }

                field = null
            }
        }

    private var bindCount = 0
        set(value) {
            field = value
            isBound = bindCount != 0
        }

    private var isBound = false
        set(value) {
            field = value
            notificationManager.lockedToForeground = value
        }

    override fun onCreate() {
        android.util.Log.d("mullvad", "MullvadVpnService.onCreate")
        super.onCreate()

        notificationManager = ForegroundNotificationManager(this, serviceNotifier)
        tunnelStateUpdater = TunnelStateUpdater(this, serviceNotifier)

        setUp()
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        android.util.Log.d("mullvad", "MullvadVpnService.onStartCommand")
        val startResult = super.onStartCommand(intent, flags, startId)
        val action = intent?.action

        if (action == VpnService.SERVICE_INTERFACE || action == KEY_CONNECT_ACTION) {
            android.util.Log.d("mullvad", "Queuing connect")
            pendingAction = PendingAction.Connect
        } else if (action == KEY_DISCONNECT_ACTION) {
            android.util.Log.d("mullvad", "Queuing disconnect")
            pendingAction = PendingAction.Disconnect
        }

        return startResult
    }

    override fun onBind(intent: Intent): IBinder {
        android.util.Log.d("mullvad", "MullvadVpnService.onBind")
        bindCount += 1
        android.util.Log.d("mullvad", "  bind count = $bindCount")

        return super.onBind(intent) ?: binder
    }

    override fun onRebind(intent: Intent) {
        android.util.Log.d("mullvad", "MullvadVpnService.onRebind")
        bindCount += 1
        android.util.Log.d("mullvad", "  bind count = $bindCount")

        if (isStopping) {
            restart()
            isStopping = false
        }
    }

    override fun onRevoke() {
        android.util.Log.d("mullvad", "MullvadVpnService.onRevoke")
        pendingAction = PendingAction.Disconnect
    }

    override fun onUnbind(intent: Intent): Boolean {
        android.util.Log.d("mullvad", "MullvadVpnService.onUnbind")
        bindCount -= 1
        android.util.Log.d("mullvad", "  bind count = $bindCount")

        return true
    }

    override fun onDestroy() {
        android.util.Log.d("mullvad", "MullvadVpnService.onDestroy")
        tearDown()
        notificationManager.onDestroy()
        super.onDestroy()
    }

    inner class LocalBinder : Binder() {
        val serviceNotifier
            get() = this@MullvadVpnService.serviceNotifier

        fun stop() {
            this@MullvadVpnService.stop()
        }
    }

    private fun setUp() {
        android.util.Log.d("mullvad", "MullvadVpnService.setUp")
        startDaemonJob?.cancel()
        startDaemonJob = startDaemon()
    }

    private fun startDaemon() = GlobalScope.launch(Dispatchers.Default) {
        android.util.Log.d("mullvad", "MullvadVpnService.startDaemon")
        prepareFiles()

        val daemon = MullvadDaemon(this@MullvadVpnService).apply {
            onDaemonStopped = {
                android.util.Log.d("mullvad", "onDaemonStopped")
                instance = null

                if (!isStopping) {
                    restart()
                }
            }
        }

        val settings = daemon.getSettings()

        if (settings != null) {
            setUpInstance(daemon, settings)
        } else {
            restart()
        }
    }

    private fun prepareFiles() {
        FileMigrator(File("/data/data/net.mullvad.mullvadvpn"), filesDir).apply {
            migrate(RELAYS_FILE)
            migrate("settings.json")
            migrate("daemon.log")
            migrate("daemon.old.log")
            migrate("wireguard.log")
            migrate("wireguard.old.log")
        }

        val shouldOverwriteRelayList =
            lastUpdatedTime() > File(filesDir, RELAYS_FILE).lastModified()

        FileResourceExtractor(this).apply {
            extract(RELAYS_FILE, shouldOverwriteRelayList)
        }
    }

    private fun setUpInstance(daemon: MullvadDaemon, settings: Settings) {
        android.util.Log.d("mullvad", "MullvadVpnService.setUpInstance")
        val settingsListener = SettingsListener(daemon, settings).apply {
            accountNumberNotifier.subscribe { accountNumber ->
                loggedIn = accountNumber != null
            }
        }

        val accountCache = AccountCache(daemon, settingsListener)

        val connectionProxy = ConnectionProxy(this@MullvadVpnService, daemon).apply {
            when (pendingAction) {
                PendingAction.Connect -> {
                    if (loggedIn) {
                        connect()
                    } else {
                        openUi()
                    }
                }
                PendingAction.Disconnect -> disconnect()
                null -> {}
            }

            pendingAction = null
        }

        val locationInfoCache = LocationInfoCache(daemon, connectionProxy, connectivityListener)

        instance = ServiceInstance(
            daemon,
            accountCache,
            connectionProxy,
            connectivityListener,
            locationInfoCache,
            settingsListener
        )
    }

    private fun stop() {
        android.util.Log.d("mullvad", "MullvadVpnService.stop")
        isStopping = true
        stopDaemon()
        stopSelf()
    }

    private fun stopDaemon() {
        android.util.Log.d("mullvad", "MullvadVpnService.stopDaemon")
        startDaemonJob?.cancel()
        instance?.daemon?.shutdown()
    }

    private fun tearDown() {
        android.util.Log.d("mullvad", "MullvadVpnService.tearDown")
        stopDaemon()
    }

    private fun restart() {
        android.util.Log.d("mullvad", "MullvadVpnService.restart")
        tearDown()
        setUp()
    }

    private fun openUi() {
        val intent = Intent(this, MainActivity::class.java).apply {
            addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
        }

        startActivity(intent)
    }

    private fun lastUpdatedTime(): Long {
        return packageManager.getPackageInfo(packageName, 0).lastUpdateTime
    }
}
