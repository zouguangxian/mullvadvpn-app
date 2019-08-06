pub struct LocationFetcher {
    requests: mpsc::Receiver<()>,
    active_request: Option<oneshot::Receiver<Option<GeoIpLocation>>>,
}

impl LocationFetcher {
    pub fn spawn() -> mpsc::Sender<()> {
        let (tx, rx) = mpsc::channel();

        thread::spawn(move || {
            let reactor = tokio_core::reactor::Core::new()
                .expect("Failed to create Tokio reactor for LocationFetcher");

            reactor.run(LocationFetcher {
                requests: tx,
                active_request: None,
            })
        });

        rx
    }
}
