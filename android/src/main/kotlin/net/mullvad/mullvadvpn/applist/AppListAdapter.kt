package net.mullvad.mullvadvpn.applist

import android.content.Context
import android.support.v7.widget.RecyclerView.Adapter
import android.view.LayoutInflater
import android.view.ViewGroup
import kotlin.properties.Delegates.observable
import net.mullvad.mullvadvpn.R
import net.mullvad.mullvadvpn.service.SplitTunnelling
import net.mullvad.mullvadvpn.util.JobTracker

class AppListAdapter(
    context: Context,
    private val splitTunnelling: SplitTunnelling
) : Adapter<AppListItemHolder>() {
    private val appList = ArrayList<AppInfo>()
    private val jobTracker = JobTracker()
    private val packageManager = context.packageManager
    private val thisPackageName = context.packageName

    var onListReady: (suspend () -> Unit)? = null

    var isListReady = false
        private set

    var enabled by observable(false) { _, oldValue, newValue ->
        if (oldValue != newValue) {
            if (newValue == true) {
                notifyItemRangeInserted(0, appList.size)
            } else {
                notifyItemRangeRemoved(0, appList.size)
            }
        }
    }

    init {
        jobTracker.newBackgroundJob("populateAppList") {
            populateAppList()
        }
    }

    override fun getItemCount() = if (enabled) { appList.size } else { 0 }

    override fun onCreateViewHolder(parentView: ViewGroup, type: Int): AppListItemHolder {
        val inflater = LayoutInflater.from(parentView.context)
        val view = inflater.inflate(R.layout.app_list_item, parentView, false)

        return AppListItemHolder(splitTunnelling, packageManager, jobTracker, view)
    }

    override fun onBindViewHolder(holder: AppListItemHolder, position: Int) {
        holder.appInfo = appList.get(position)
    }

    private fun populateAppList() {
        val applications = packageManager
            .getInstalledApplications(0)
            .filter { info -> info.packageName != thisPackageName }
            .map { info -> AppInfo(info, packageManager.getApplicationLabel(info).toString()) }

        appList.apply {
            clear()
            addAll(applications)
            sortBy { info -> info.label }
        }

        jobTracker.newUiJob("notifyAppListChanges") {
            isListReady = true
            onListReady?.invoke()
            notifyItemRangeInserted(0, applications.size)
        }
    }
}
