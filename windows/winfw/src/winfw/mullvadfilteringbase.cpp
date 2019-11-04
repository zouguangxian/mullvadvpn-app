#include "stdafx.h"
#include "mullvadfilteringbase.h"
#include "mullvadguids.h"
#include "libwfp/objectinstaller.h"
#include "libwfp/objectexplorer.h"
#include "libwfp/objectenumerator.h"
#include "libwfp/objectdeleter.h"


//static
std::unique_ptr<wfp::ProviderBuilder> MullvadFilteringBase::Provider()
{
	auto builder = std::make_unique<wfp::ProviderBuilder>();

	(*builder)
		.name(L"Mullvad VPN")
		.description(L"Mullvad VPN firewall integration")
		//.persistent()
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
		//.persistent()
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
		//.persistent()
		.weight(MAXUINT16 - 1);

	return builder;
}

//static
void MullvadFilteringBase::Init(wfp::FilterEngine& engine)
{
	const auto providerBuilder = Provider();
	providerBuilder->persistent();

	if (!wfp::ObjectExplorer::GetProvider(
		engine,
		providerBuilder->id(),
		[](const FWPM_PROVIDER0&) { return true; }
	))
	{
		wfp::ObjectInstaller::AddProvider(engine, *providerBuilder);
	}

	const auto whitelistBuilder = SublayerWhitelist();
	whitelistBuilder->persistent();

	if (!wfp::ObjectExplorer::GetSublayer(
		engine,
		whitelistBuilder->id(),
		[](const FWPM_SUBLAYER0&) { return true; }
	))
	{
		wfp::ObjectInstaller::AddSublayer(engine, *whitelistBuilder);
	}

	// TODO: update existing objects (without deleting them, if possible)
}

void MullvadFilteringBase::Purge(wfp::FilterEngine &engine)
{
	//
	// Delete all filters
	//
	wfp::ObjectEnumerator::Filters(
		engine,
		[&engine](const FWPM_FILTER0 &filter)
		{
			if (0 == memcmp(filter.providerKey, &MullvadGuids::Provider(), sizeof(GUID)))
			{
				wfp::ObjectDeleter::DeleteFilter(engine, filter.filterKey);
			}
			return true;
		}
	);

	//
	// Delete all sublayers
	//
	wfp::ObjectEnumerator::Sublayers(
		engine,
		[&engine](const FWPM_SUBLAYER0 &sublayer)
		{
			if (0 == memcmp(sublayer.providerKey, &MullvadGuids::Provider(), sizeof(GUID)))
			{
				wfp::ObjectDeleter::DeleteSublayer(engine, sublayer.subLayerKey);
			}
			return true;
		}
	);

	//
	// Delete provider
	//
	wfp::ObjectDeleter::DeleteProvider(engine, MullvadGuids::Provider());
}
