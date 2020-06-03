/// Errors that may occur in [`SplitTunnel`].
#[derive(err_derive::Error, Debug)]
pub enum Error {
    /// The Windows DLL or kernel-mode driver returned an error.
    #[error(display = "Failed to initialize split tunneling")]
    InitializationFailed,

    /// Failed to update the paths to exclude from the tunnel.
    #[error(display = "Failed to update exclusions: {}", _0)]
    UpdatePaths(winexclude::WinExcludeUpdateStatus),
}

/// Manages applications whose traffic to exclude from the tunnel.
pub struct SplitTunnel {
    paths: Vec<String>,
}

impl SplitTunnel {
    /// Initialize the driver.
    pub fn new() -> Result<Self, Error> {
        // WinExclude_Initialize();
        Ok(SplitTunnel { paths: vec![] })
    }

    /// Set a list of applications to exclude from the tunnel.
    pub fn set_paths<T: AsRef<str>>(&mut self, paths: &[T]) -> Result<(), Error> {
        self.paths.clear();
        for path in paths {
            self.paths.push(path.as_ref().to_string());
        }

        // WinExclude_SetAppPaths
        Ok(())
    }
}

impl Drop for SplitTunnel {
    fn drop(&mut self) {
        // WinExclude_Deinitialize();
    }
}

#[allow(non_snake_case)]
mod winexclude {
    use crate::logging::windows::LogSink;
    use std::fmt;

    #[allow(dead_code)]
    #[repr(u32)]
    #[derive(Debug, Clone, Copy)]
    pub enum WinExcludeUpdateStatus {
        Success = 0,
        NotFound = 1,
        InvalidArgument = 2,
    }

    impl Into<Result<(), super::Error>> for WinExcludeUpdateStatus {
        fn into(self) -> Result<(), super::Error> {
            match self {
                WinExcludeUpdateStatus::Success => Ok(()),
                val => Err(super::Error::UpdatePaths(val)),
            }
        }
    }

    impl fmt::Display for WinExcludeUpdateStatus {
        fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
            use WinExcludeUpdateStatus::*;
            write!(
                f,
                "{}",
                match self {
                    Success => "Updated exclusions paths",
                    NotFound => "One or more paths were not found",
                    InvalidArgument => "Invalid argument",
                }
            )
        }
    }

    extern "system" {
        #[link_name = "WinExclude_Initialize"]
        pub fn WinExclude_Initialize(sink: Option<LogSink>, sink_context: *const u8) -> bool;

        #[link_name = "WinExclude_Deinitialize"]
        pub fn WinExclude_Deinitialize() -> bool;

        #[link_name = "WinExclude_SetAppPaths"]
        pub fn WinExclude_SetAppPaths(paths: *const *const u16) -> WinExcludeUpdateStatus;
    }
}
