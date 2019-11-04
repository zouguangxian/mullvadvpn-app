#include "stdafx.h"
#include "persistentblock.h"
#include "mullvadguids.h"
#include "libwfp/filterbuilder.h"
#include "libwfp/nullconditionbuilder.h"
#include "libwfp/objectinstaller.h"
#include "libwfp/objectdeleter.h"
#include "mullvadfilteringbase.h"


//static
const GUID& PersistentBlock::PersistentFilterBlockAll_Outbound_Ipv4()
{
	// c2114024-1274-4333-9169-670caff8a987
	static const GUID g =
	{
		0xc2114024,
		0x1274,
		0x4333,
		{ 0x91, 0x69, 0x67, 0x0c, 0xaf, 0xf8, 0xa9, 0x87 }
	};

	return g;
}

//static
const GUID& PersistentBlock::PersistentFilterBlockAll_Inbound_Ipv4()
{
	// c31d3889-2c5e-42b3-9617-676e4e354ae7
	static const GUID g =
	{
		0xc31d3889,
		0x2c5e,
		0x42b3,
		{ 0x96, 0x17, 0x67, 0x6e, 0x4e, 0x35, 0x4a, 0xe7 }
	};

	return g;
}

//static
const GUID& PersistentBlock::BootTimeFilterBlockAll_Outbound_Ipv4()
{
	// f5f83fe4-5273-4661-8ab4-c981a8bad70b
	static const GUID g =
	{
		0xf5f83fe4,
		0x5273,
		0x4661,
		{ 0x8a, 0xb4, 0xc9, 0x81, 0xa8, 0xba, 0xd7, 0x0b }
	};

	return g;
}

//static
const GUID& PersistentBlock::BootTimeFilterBlockAll_Inbound_Ipv4()
{
	// 8ab109ee-c733-4477-a203-172eec62829f
	static const GUID g =
	{
		0x8ab109ee,
		0xc733,
		0x4477,
		{ 0xa2, 0x03, 0x17, 0x2e, 0xec, 0x62, 0x82, 0x9f }
	};

	return g;
}


bool PersistentBlock::Enable(wfp::FilterEngine& engine)
{
	if (!wfp::ObjectInstaller::AddProvider(engine, *MullvadFilteringBase::Provider()))
	{
		return false;
	}

	if (!wfp::ObjectInstaller::AddSublayer(engine, *MullvadFilteringBase::SublayerWhitelist()))
	{
		return false;
	}

	wfp::FilterBuilder filterBuilder;

	//
	// Create persistent BFE filters
	//

	// Block IPv4 traffic

	filterBuilder
		.key(PersistentFilterBlockAll_Outbound_Ipv4())
		.name(L"Block all outbound connections (IPv4)")
		.description(L"This filter is part of a rule that restricts inbound and outbound traffic")
		.provider(MullvadGuids::Provider())
		.layer(FWPM_LAYER_ALE_AUTH_CONNECT_V4)
		.sublayer(MullvadGuids::SublayerWhitelist())
		.weight(wfp::FilterBuilder::WeightClass::Min)
		.persistent()
		.block();

	const wfp::NullConditionBuilder nullConditionBuilder;

	if (!wfp::ObjectInstaller::AddFilter(engine, filterBuilder, nullConditionBuilder))
	{
		return false;
	}

	filterBuilder
		.key(PersistentFilterBlockAll_Inbound_Ipv4())
		.name(L"Block all inbound connections (IPv4)")
		.layer(FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4);

	if (!wfp::ObjectInstaller::AddFilter(engine, filterBuilder, nullConditionBuilder))
	{
		return false;
	}

	// TODO: add IPv6 filters

	//
	// Create boot-time filters (before BFE starts)
	//

	// Block IPv4 traffic

	filterBuilder
		.key(BootTimeFilterBlockAll_Outbound_Ipv4())
		.name(L"Block all outbound connections (IPv4)")
		.layer(FWPM_LAYER_ALE_AUTH_CONNECT_V4)
		.sublayer(MullvadGuids::SublayerWhitelist())
		.weight(wfp::FilterBuilder::WeightClass::Min)
		.boottime();

	if (!wfp::ObjectInstaller::AddFilter(engine, filterBuilder, nullConditionBuilder))
	{
		return false;
	}

	filterBuilder
		.key(BootTimeFilterBlockAll_Inbound_Ipv4())
		.name(L"Block all inbound connections (IPv4)")
		.layer(FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4);

	if (!wfp::ObjectInstaller::AddFilter(engine, filterBuilder, nullConditionBuilder))
	{
		return false;
	}

	// TODO: add IPv6 filters

	// TODO: permit LAN

	return true;
}

bool PersistentBlock::Disable(wfp::FilterEngine& engine)
{
	wfp::ObjectDeleter::DeleteFilter(engine, PersistentFilterBlockAll_Inbound_Ipv4());
	wfp::ObjectDeleter::DeleteFilter(engine, PersistentFilterBlockAll_Outbound_Ipv4());
	wfp::ObjectDeleter::DeleteFilter(engine, BootTimeFilterBlockAll_Inbound_Ipv4());
	wfp::ObjectDeleter::DeleteFilter(engine, BootTimeFilterBlockAll_Outbound_Ipv4());

	// TODO: remove IPv6 filters

	// TODO: remove boot-time filters

	wfp::ObjectDeleter::DeleteSublayer(engine, MullvadGuids::SublayerWhitelist());
	
	wfp::ObjectDeleter::DeleteProvider(engine, MullvadGuids::Provider());

	return true;
}
