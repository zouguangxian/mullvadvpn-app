#include "stdafx.h"
#include "objectpurger.h"
#include "mullvadguids.h"
#include "wfpobjecttype.h"
#include "libwfp/filterengine.h"
#include "libwfp/objectdeleter.h"
#include "libwfp/transaction.h"
#include <algorithm>
#include "libwfp/objectexplorer.h"

namespace
{

using ObjectDeleter = std::function<void(wfp::FilterEngine &, const GUID &)>;

template<typename TRange>
void RemoveRange(wfp::FilterEngine &engine, ObjectDeleter deleter, TRange range)
{
	std::for_each(range.first, range.second, [&](const auto &record)
	{
		const GUID &objectId = record.second;
		deleter(engine, objectId);
	});
}

template<typename TRange, typename Predicate>
void RemoveRangeIf(wfp::FilterEngine &engine, ObjectDeleter deleter, TRange range, Predicate q)
{
	std::for_each(range.first, range.second, [&](const auto &record)
	{
		const GUID &objectId = record.second;
		if (q(objectId))
		{
			deleter(engine, objectId);
		}
	});
}

} // anonymous namespace

//static
ObjectPurger::RemovalFunctor ObjectPurger::GetRemoveAllNonPersistentFunctor()
{
	return [](wfp::FilterEngine &engine)
	{
		const auto registry = MullvadGuids::DetailedRegistry();

		// Resolve correct overload.
		void(*deleter)(wfp::FilterEngine &, const GUID &) = wfp::ObjectDeleter::DeleteFilter;

		RemoveRangeIf(
			engine,
			deleter,
			registry.equal_range(WfpObjectType::Filter),
			[&engine](const GUID &guid)
			{
				return wfp::ObjectExplorer::GetFilter(engine, guid, [](const FWPM_FILTER0 &filter)
				{
					return !(filter.flags & FWPM_FILTER_FLAG_PERSISTENT);
				});
			}
		);
		RemoveRangeIf(
			engine,
			wfp::ObjectDeleter::DeleteSublayer,
			registry.equal_range(WfpObjectType::Sublayer),
			[&engine](const GUID &guid)
			{
				return wfp::ObjectExplorer::GetSublayer(engine, guid, [](const FWPM_SUBLAYER0 &sublayer)
				{
					return !(sublayer.flags & FWPM_SUBLAYER_FLAG_PERSISTENT);
				});
			}
		);
		RemoveRangeIf(
			engine, wfp::ObjectDeleter::DeleteProvider,
			registry.equal_range(WfpObjectType::Provider),
			[&engine](const GUID &guid)
			{
				return wfp::ObjectExplorer::GetProvider(engine, guid, [](const FWPM_PROVIDER0 &provider)
				{
					return !(provider.flags & FWPM_PROVIDER_FLAG_PERSISTENT);
				});
			}
		);
	};
}

//static
ObjectPurger::RemovalFunctor ObjectPurger::GetRemoveAllFunctor()
{
	return [](wfp::FilterEngine &engine)
	{
		const auto registry = MullvadGuids::DetailedRegistry();

		// Resolve correct overload.
		void(*deleter)(wfp::FilterEngine &, const GUID &) = wfp::ObjectDeleter::DeleteFilter;

		RemoveRange(engine, deleter, registry.equal_range(WfpObjectType::Filter));
		RemoveRange(engine, wfp::ObjectDeleter::DeleteSublayer, registry.equal_range(WfpObjectType::Sublayer));
		RemoveRange(engine, wfp::ObjectDeleter::DeleteProvider, registry.equal_range(WfpObjectType::Provider));
	};
}

//static
bool ObjectPurger::Execute(RemovalFunctor f)
{
	auto engine = wfp::FilterEngine::StandardSession();

	auto wrapper = [&]()
	{
		return f(*engine), true;
	};

	return wfp::Transaction::Execute(*engine, wrapper);
}
