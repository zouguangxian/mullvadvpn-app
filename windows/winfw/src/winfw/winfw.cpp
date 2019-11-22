#include "stdafx.h"
#include "libwfp/transaction.h"
#include "winfw.h"
#include "fwcontext.h"
#include "objectpurger.h"
#include "mullvadfilteringbase.h"
#include "persistentblock.h"
#include <windows.h>
#include <stdexcept>
#include <optional>

namespace
{

uint32_t g_timeout = 0;

WinFwErrorSink g_errorSink = nullptr;
void * g_errorContext = nullptr;

FwContext *g_fwContext = nullptr;

std::optional<FwContext::PingableHosts> ConvertPingableHosts(const PingableHosts* pingableHosts)
{
	if (nullptr == pingableHosts)
	{
		return {};
	}

	if (nullptr == pingableHosts->hosts
		|| 0 == pingableHosts->numHosts)
	{
		throw std::runtime_error("Invalid PingableHosts structure");
	}

	FwContext::PingableHosts converted;

	if (nullptr != pingableHosts->tunnelInterfaceAlias)
	{
		converted.tunnelInterfaceAlias = pingableHosts->tunnelInterfaceAlias;
	}

	for (size_t i = 0; i < pingableHosts->numHosts; ++i)
	{
		converted.hosts.emplace_back(wfp::IpAddress(pingableHosts->hosts[i]));
	}

	return converted;
}

void InitializeWfpState()
{
	auto engine = wfp::FilterEngine::StandardSession(g_timeout);

	// Reset boot-time and persistent objects, if they exist
	if (!wfp::Transaction::Execute(*engine, [&engine]()
	{
		MullvadFilteringBase::Init(*engine);
		if (!PersistentBlock::Disable(*engine))
		{
			return false;
		}
		return true;
	}
	))
	{
		throw std::runtime_error("Failed to remove boot-time configuration.");
	}
}

} // anonymous namespace

WINFW_LINKAGE
bool
WINFW_API
WinFw_Initialize(
	uint32_t timeout,
	WinFwErrorSink errorSink,
	void *errorContext
)
{
	if (nullptr != g_fwContext)
	{
		//
		// This is an error.
		// The existing instance may have a different timeout etc.
		//
		return false;
	}

	// Convert seconds to milliseconds.
	g_timeout = timeout * 1000;

	g_errorSink = errorSink;
	g_errorContext = errorContext;

	try
	{
		InitializeWfpState();
		g_fwContext = new FwContext(g_timeout);
	}
	catch (std::exception &err)
	{
		if (nullptr != g_errorSink)
		{
			g_errorSink(err.what(), g_errorContext);
		}

		return false;
	}
	catch (...)
	{
		return false;
	}

	return true;
}

extern "C"
WINFW_LINKAGE
bool
WINFW_API
WinFw_InitializeBlocked(
	uint32_t timeout,
	const WinFwSettings &settings,
	WinFwErrorSink errorSink,
	void *errorContext
)
{
	if (nullptr != g_fwContext)
	{
		//
		// This is an error.
		// The existing instance may have a different timeout etc.
		//
		return false;
	}

	// Convert seconds to milliseconds.
	g_timeout = timeout * 1000;

	g_errorSink = errorSink;
	g_errorContext = errorContext;

	try
	{
		InitializeWfpState();
		g_fwContext = new FwContext(g_timeout, settings);
	}
	catch (std::exception &err)
	{
		if (nullptr != g_errorSink)
		{
			g_errorSink(err.what(), g_errorContext);
		}

		return false;
	}
	catch (...)
	{
		return false;
	}

	return true;
}

WINFW_LINKAGE
bool
WINFW_API
WinFw_Deinitialize(bool addBootTimeFilters)
{
	bool status = false;

	if (addBootTimeFilters)
	{
		try
		{
			const auto engine = wfp::FilterEngine::StandardSession(g_timeout);
			status = wfp::Transaction::Execute(*engine, [&engine]()
			{
				return PersistentBlock::Enable(*engine);
			});
		}
		catch (std::exception & err)
		{
			if (nullptr != g_errorSink)
			{
				g_errorSink(err.what(), g_errorContext);
			}

			status = false;
		}
	}
	else
	{
		status = true;
	}

	if (nullptr == g_fwContext)
	{
		return status;
	}

	delete g_fwContext;
	g_fwContext = nullptr;

	return status;
}

WINFW_LINKAGE
bool
WINFW_API
WinFw_ApplyPolicyConnecting(
	const WinFwSettings &settings,
	const WinFwRelay &relay,
	const PingableHosts *pingableHosts
)
{
	if (nullptr == g_fwContext)
	{
		return false;
	}

	try
	{
		return g_fwContext->applyPolicyConnecting(settings, relay, ConvertPingableHosts(pingableHosts));
	}
	catch (std::exception &err)
	{
		if (nullptr != g_errorSink)
		{
			g_errorSink(err.what(), g_errorContext);
		}

		return false;
	}
	catch (...)
	{
		return false;
	}
}

WINFW_LINKAGE
bool
WINFW_API
WinFw_ApplyPolicyConnected(
	const WinFwSettings &settings,
	const WinFwRelay &relay,
	const wchar_t *tunnelInterfaceAlias,
	const wchar_t *v4DnsHost,
	const wchar_t *v6DnsHost
)
{
	if (nullptr == g_fwContext)
	{
		return false;
	}

	try
	{
		return g_fwContext->applyPolicyConnected(settings, relay, tunnelInterfaceAlias, v4DnsHost, v6DnsHost);
	}
	catch (std::exception &err)
	{
		if (nullptr != g_errorSink)
		{
			g_errorSink(err.what(), g_errorContext);
		}

		return false;
	}
	catch (...)
	{
		return false;
	}
}

WINFW_LINKAGE
bool
WINFW_API
WinFw_ApplyPolicyBlocked(
	const WinFwSettings &settings
)
{
	if (nullptr == g_fwContext)
	{
		return false;
	}

	try
	{
		return g_fwContext->applyPolicyBlocked(settings);
	}
	catch (std::exception &err)
	{
		if (nullptr != g_errorSink)
		{
			g_errorSink(err.what(), g_errorContext);
		}

		return false;
	}
	catch (...)
	{
		return false;
	}
}

WINFW_LINKAGE
bool
WINFW_API
WinFw_Reset()
{
	try
	{
		if (nullptr == g_fwContext)
		{
			return ObjectPurger::Execute(ObjectPurger::GetRemoveAllFunctor());
		}
		else
		{
			return g_fwContext->reset();
		}
	}
	catch (std::exception &err)
	{
		if (nullptr != g_errorSink)
		{
			g_errorSink(err.what(), g_errorContext);
		}

		return false;
	}
	catch (...)
	{
		return false;
	}
}

WINFW_LINKAGE
bool
WINFW_API
WinFw_Purge()
{
	try
	{
		const auto session = wfp::FilterEngine::StandardSession();
		MullvadFilteringBase::Purge(*session);
		return true;
	}
	catch (std::exception &err)
	{
		if (nullptr != g_errorSink)
		{
			g_errorSink(err.what(), g_errorContext);
		}

		return false;
	}
	catch (...)
	{
		return false;
	}
}
