/// Errors that may occur in [`SplitTunnel`].
#[derive(err_derive::Error, Debug)]
pub enum Error {
    /// The Windows DLL or kernel-mode driver returned an error.
    #[error(display = "Failed to initialize split tunneling")]
    InitializationFailed,
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
