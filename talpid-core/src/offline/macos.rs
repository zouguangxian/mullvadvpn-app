use crate::tunnel_state_machine::TunnelCommand;
use block::{Block, ConcreteBlock, RcBlock};
use futures01::sync::mpsc::UnboundedSender;
// use network_framework_sys::{
//     nw_interface_get_type, nw_interface_t, nw_interface_type_cellular, nw_interface_type_wifi,
//     nw_interface_type_wired, nw_path_monitor_cancel, nw_path_monitor_create,
// nw_path_monitor_start,     nw_path_monitor_t, nw_path_t, nw_release,
// };
use network_framework_sys::*;
use std::{
    cell::RefCell,
    process::Command,
    rc::Rc,
    sync::{
        atomic::{AtomicU32, Ordering},
        Arc, Weak,
    },
};

// Allowing improper C-Types since Block<_> is FFI safe, it's whole purpose is to aid FFI with
// Objective-C.
#[allow(improper_ctypes)]
extern "C" {
    fn nw_path_monitor_set_update_handler(
        monitor: nw_path_monitor_t,
        handler: *mut Block<(nw_path_t,), ()>,
    );

    fn nw_path_enumerate_interfaces(path: nw_path_t, handler: *mut Block<(nw_interface_t,), bool>);
}


#[derive(err_derive::Error, Debug)]
pub enum Error {
    #[error(display = "Failed to initialize path watcher")]
    PathMonitorInitError,
}

pub struct MonitorHandle {
    monitor: nw_path_monitor_t,
    num_valid_interfaces: Arc<AtomicU32>,
    _callback: RcBlock<(nw_path_t,), ()>,
}

pub fn spawn_monitor(sender: Weak<UnboundedSender<TunnelCommand>>) -> Result<MonitorHandle, Error> {
    let monitor = unsafe { nw_path_monitor_create() };
    if monitor.is_null() {
        return Err(Error::PathMonitorInitError);
    }

    let initial_num_interfaces = if has_any_default_route() { 1 } else { 0 };
    let num_valid_interfaces = Arc::new(AtomicU32::new(initial_num_interfaces));
    let n = num_valid_interfaces.clone();

    let callback = ConcreteBlock::new(move |path| {
        let num_interfaces = num_connected_interfaces(path);
        let old_num_interfaces = n.swap(num_interfaces, Ordering::SeqCst);
        if (num_interfaces < 1) != (old_num_interfaces < 1) {
            if let Some(tx) = sender.upgrade() {
                let is_offline = num_interfaces < 1;
                let _ = tx.unbounded_send(TunnelCommand::IsOffline(is_offline));
            }
        }
    })
    .copy();

    unsafe {
        nw_path_monitor_set_update_handler(monitor, &*callback as *const _ as *mut _);
        nw_path_monitor_start(monitor);
    }

    Ok(MonitorHandle {
        monitor,
        _callback: callback,
        num_valid_interfaces,
    })
}

impl MonitorHandle {
    /// Host is considered to be offline if there are no active interfaces that could provide
    /// connectivity.
    pub fn is_offline(&self) -> bool {
        self.num_valid_interfaces.load(Ordering::SeqCst) < 1
    }
}

/// Returns true if the host has either an IPv4 or an IPv6 or both default routes or if `route get
/// default` fails
fn has_any_default_route() -> bool {
    get_connectivity("-inet") || get_connectivity("-inet6")
}

fn get_connectivity(af_family: &'static str) -> bool {
    let mut cmd = Command::new("route");
    cmd.arg("-n").arg("get").arg(af_family).arg("default");
    match cmd.output() {
        Err(_) => {
            log::error!("Failed to get default route for {} to determine connectivity, assuming machine isn't offline", af_family);
            true
        }
        Ok(output) => {
            // if stderr is not empty, safely assuming that there is no default route
            output.stderr.is_empty()
        }
    }
}


// Unsafe `Send` implementation is required as `Block<_>` does not implement Send. But since it's
// allocated on the heap and it's only held in the `MonitorHandle` to ensure that its deallocated,
// there's no reason not to send it across threads.
unsafe impl Send for MonitorHandle {}

impl Drop for MonitorHandle {
    fn drop(&mut self) {
        unsafe {
            nw_path_monitor_cancel(self.monitor);
            nw_release(self.monitor as *const _ as *mut _);
        }
    }
}


/// Returns the number of interfaces that provie IPv4/IPv6 connectivity for a given path, and are
/// not tunnels
fn num_connected_interfaces(path: nw_path_t) -> u32 {
    let num_ifaces = Rc::new(RefCell::new(0));
    let result = Rc::clone(&num_ifaces);

    let enumeration_callback = ConcreteBlock::new(move |interface| {
        let interface_type = unsafe { nw_interface_get_type(interface) };
        if interface_type == nw_interface_type_wifi
            || interface_type == nw_interface_type_wired
            || interface_type == nw_interface_type_cellular
        {
            *num_ifaces.borrow_mut() += 1;
        }
        true
    });

    unsafe { nw_path_enumerate_interfaces(path, &*enumeration_callback as *const _ as *mut _) };
    return *result.borrow();
}

#[cfg(test)]
mod test {
    use super::*;
    use std::{
        cell::RefCell,
        rc::Rc,
        sync::{
            atomic::{AtomicU32, Ordering},
            mpsc, Arc, Weak,
        },
        thread,
    };


    #[test]
    fn test_path_inspection() {
        let monitor = unsafe { nw_path_monitor_create() };
        assert!(!monitor.is_null());


        let num = Arc::new(AtomicU32::new(0));
        let n = num.clone();
        let cb = ConcreteBlock::new(move |path| {
            n.store(num_connected_interfaces(path), Ordering::SeqCst);
        })
        .copy();
        unsafe {
            nw_path_monitor_set_update_handler(monitor, &*cb as *const _ as *mut _);
            nw_path_monitor_start(monitor);
            nw_path_monitor_cancel(monitor);
        }

        assert_eq!(0, num.load(Ordering::SeqCst));
        let monitor = unsafe { nw_path_monitor_create() };
        assert!(!monitor.is_null());


        let num = Arc::new(AtomicU32::new(0));
        let n = num.clone();
        let cb = ConcreteBlock::new(move |path| {
            n.store(num_connected_interfaces(path), Ordering::SeqCst);
        })
        .copy();
        unsafe {
            nw_path_monitor_set_update_handler(monitor, &*cb as *const _ as *mut _);
            nw_path_monitor_start(monitor);
            nw_path_monitor_cancel(monitor);
        }

        assert_eq!(0, num.load(Ordering::SeqCst));
        let monitor = unsafe { nw_path_monitor_create() };
        assert!(!monitor.is_null());


        let num = Arc::new(AtomicU32::new(0));
        let n = num.clone();
        let cb = ConcreteBlock::new(move |path| {
            n.store(num_connected_interfaces(path), Ordering::SeqCst);
        })
        .copy();
        unsafe {
            nw_path_monitor_set_update_handler(monitor, &*cb as *const _ as *mut _);
            nw_path_monitor_start(monitor);
            nw_path_monitor_cancel(monitor);
        }

        assert_eq!(0, num.load(Ordering::SeqCst));
    }

    #[test]
    fn test_initial_path() {
        let _ = get_initial_path();
    }
}
