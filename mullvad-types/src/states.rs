use crate::location::GeoIpLocation;
#[cfg(target_os = "android")]
use jnix::IntoJava;
use serde::{Deserialize, Serialize};
use talpid_types::{
    net::TunnelEndpoint,
    tunnel::{ActionAfterDisconnect, ErrorState},
};

/// Represents the state the client strives towards.
/// When in `Secured`, the client should keep the computer from leaking and try to
/// establish a VPN tunnel if it is not up.
#[derive(Debug, Copy, Clone, Eq, PartialEq, Hash, Serialize, Deserialize)]
#[serde(rename_all = "snake_case")]
pub enum TargetState {
    Unsecured,
    Secured,
}

/// Represents the state the client tunnel is in.
#[derive(Debug, Clone, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "snake_case")]
#[serde(tag = "state", content = "details")]
#[cfg_attr(target_os = "android", derive(IntoJava))]
#[cfg_attr(target_os = "android", jnix(package = "net.mullvad.mullvadvpn.model"))]
pub enum TunnelState {
    Disconnected,
    Connecting {
        endpoint: TunnelEndpoint,
        location: Option<GeoIpLocation>,
    },
    Connected {
        endpoint: TunnelEndpoint,
        location: Option<GeoIpLocation>,
    },
    Disconnecting(ActionAfterDisconnect),
    Error(ErrorState),
}

impl TunnelState {
    /// Returns true if the tunnel state is in the error state.
    pub fn is_in_error_state(&self) -> bool {
        match self {
            TunnelState::Error(_) => true,
            _ => false,
        }
    }
}
