use jnix::jni::{objects::GlobalRef, JavaVM};
use std::sync::{atomic::AtomicBool, Arc};

#[derive(Clone)]
pub struct AndroidContext {
    pub jvm: Arc<JavaVM>,
    pub vpn_service: GlobalRef,
    pub replace_tun: Arc<AtomicBool>,
}
