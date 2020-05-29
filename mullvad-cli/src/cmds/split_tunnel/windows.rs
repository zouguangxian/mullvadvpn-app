use crate::{new_rpc_client, Command, Result};
use clap::value_t_or_exit;

pub struct SplitTunnel;

impl Command for SplitTunnel {
    fn name(&self) -> &'static str {
        "split-tunnel"
    }

    fn clap_subcommand(&self) -> clap::App<'static, 'static> {
        clap::SubCommand::with_name(self.name())
            .about("Manage applications to exclude from the tunnel")
            .setting(clap::AppSettings::SubcommandRequiredElseHelp)
            .subcommand(clap::SubCommand::with_name("list"))
            .subcommand(
                clap::SubCommand::with_name("add").arg(clap::Arg::with_name("path").required(true)),
            )
            .subcommand(
                clap::SubCommand::with_name("remove")
                    .arg(clap::Arg::with_name("path").required(true)),
            )
    }

    fn run(&self, matches: &clap::ArgMatches<'_>) -> Result<()> {
        match matches.subcommand() {
            ("list", Some(_)) => {
                let paths = new_rpc_client()?.get_split_tunnel_apps()?;

                println!("Excluded applications:");
                for path in paths {
                    println!("    {}", path);
                }

                Ok(())
            }
            ("add", Some(matches)) => {
                let path = value_t_or_exit!(matches.value_of("path"), String);
                new_rpc_client()?.add_split_tunnel_app(path)?;
                Ok(())
            }
            ("remove", Some(matches)) => {
                let path = value_t_or_exit!(matches.value_of("path"), String);
                new_rpc_client()?.remove_split_tunnel_app(path)?;
                Ok(())
            }
            _ => unreachable!("unhandled subcommand"),
        }
    }
}
