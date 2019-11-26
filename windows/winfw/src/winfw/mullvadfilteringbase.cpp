#include "stdafx.h"
#include "mullvadfilteringbase.h"
#include "mullvadguids.h"
#include "libwfp/objectinstaller.h"
#include "libwfp/objectexplorer.h"
#include "libwfp/objectenumerator.h"
#include "libwfp/objectdeleter.h"



//static
const GUID& MullvadFilteringBase::ProviderGuid()
{
	static const GUID g =
	{
		0x21e1dab8,
		0xb9db,
		0x43c0,
		{ 0xb3, 0x43, 0xeb, 0x93, 0x65, 0xc7, 0xbd, 0xd2 }
	};

	return g;
}

//static
const GUID& MullvadFilteringBase::SublayerWhitelistGuid()
{
	static const GUID g =
	{
		0x11d1a31a,
		0xd7fa,
		0x469b,
		{ 0xbc, 0x21, 0xcc, 0xe9, 0x2e, 0x35, 0xfe, 0x90 }
	};

	return g;
}

//static
std::unique_ptr<wfp::ProviderBuilder> MullvadFilteringBase::Provider()
{
	auto builder = std::make_unique<wfp::ProviderBuilder>();

	(*builder)
		.name(L"Mullvad VPN")
		.description(L"Mullvad VPN firewall integration")
		.persistent()
		.key(ProviderGuid());

	return builder;
}

//static
std::unique_ptr<wfp::SublayerBuilder> MullvadFilteringBase::SublayerWhitelist()
{
	auto builder = std::make_unique<wfp::SublayerBuilder>();

	(*builder)
		.name(L"Mullvad VPN whitelist")
		.description(L"Filters that permit traffic")
		.key(SublayerWhitelistGuid())
		.provider(ProviderGuid())
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
		.provider(ProviderGuid())
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
			if (nullptr != filter.providerKey
				&& 0 == memcmp(filter.providerKey, &ProviderGuid(), sizeof(GUID)))
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
			if (nullptr != sublayer.providerKey
				&& 0 == memcmp(sublayer.providerKey, &ProviderGuid(), sizeof(GUID)))
			{
				wfp::ObjectDeleter::DeleteSublayer(engine, sublayer.subLayerKey);
			}
			return true;
		}
	);

	//
	// Delete provider
	//
	wfp::ObjectDeleter::DeleteProvider(engine, ProviderGuid());
}
