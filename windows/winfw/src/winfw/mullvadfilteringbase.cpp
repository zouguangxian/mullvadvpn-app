#include "stdafx.h"
#include "mullvadfilteringbase.h"
#include "mullvadguids.h"
#include "libwfp/objectinstaller.h"
#include "libwfp/objectexplorer.h"


//static
std::unique_ptr<wfp::ProviderBuilder> MullvadFilteringBase::Provider()
{
	auto builder = std::make_unique<wfp::ProviderBuilder>();

	(*builder)
		.name(L"Mullvad VPN")
		.description(L"Mullvad VPN firewall integration")
		.persistent()
		.key(MullvadGuids::Provider());

	return builder;
}

//static
std::unique_ptr<wfp::SublayerBuilder> MullvadFilteringBase::SublayerWhitelist()
{
	auto builder = std::make_unique<wfp::SublayerBuilder>();

	(*builder)
		.name(L"Mullvad VPN whitelist")
		.description(L"Filters that permit traffic")
		.key(MullvadGuids::SublayerWhitelist())
		.provider(MullvadGuids::Provider())
		.persistent()
		.weight(MAXUINT16);

	return builder;
}

//static
std::unique_ptr<wfp::SublayerBuilder> MullvadFilteringBase::SublayerBlacklist()
{
	auto builder = std::make_unique<wfp::SublayerBuilder>();

	(*builder)
		.name(L"Mullvad VPN blacklist")
		.description(L"Filters that block traffic")
		.key(MullvadGuids::SublayerBlacklist())
		.provider(MullvadGuids::Provider())
		.persistent()
		.weight(MAXUINT16 - 1);

	return builder;
}

//static
void MullvadFilteringBase::Init(wfp::FilterEngine& engine)
{
	const auto providerBuilder = Provider();

	if (!wfp::ObjectExplorer::GetProvider(
		engine,
		providerBuilder->id(),
		[](const FWPM_PROVIDER0&) { return true; }
	))
	{
		wfp::ObjectInstaller::AddProvider(engine, *providerBuilder);
	}

	const auto whitelistBuilder = SublayerWhitelist();

	if (!wfp::ObjectExplorer::GetSublayer(
		engine,
		whitelistBuilder->id(),
		[](const FWPM_SUBLAYER0&) { return true; }
	))
	{
		wfp::ObjectInstaller::AddSublayer(engine, *whitelistBuilder);
	}

	const auto blacklistBuilder = SublayerBlacklist();

	if (!wfp::ObjectExplorer::GetSublayer(
		engine,
		blacklistBuilder->id(),
		[](const FWPM_SUBLAYER0&) { return true; }
	))
	{
		wfp::ObjectInstaller::AddSublayer(engine, *blacklistBuilder);
	}

	// TODO: update existing objects (without deleting them, if possible)
}
