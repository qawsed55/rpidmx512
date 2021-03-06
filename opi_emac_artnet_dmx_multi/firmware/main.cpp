/**
 * @file main.cpp
 *
 */
/* Copyright (C) 2018-2019 by Arjan van Vught mailto:info@raspberrypi-dmx.nl
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdio.h>
#include <stdint.h>

#include "hardware.h"
#include "networkh3emac.h"
#include "ledblink.h"

#include "console.h"

#include "displayudf.h"
#include "displayudfparams.h"
#include "storedisplayudf.h"

#include "networkconst.h"
#include "artnetconst.h"

#include "artnet4node.h"
#include "artnet4params.h"

#include "artnetdiscovery.h"

#include "ipprog.h"
#include "displayudfhandler.h"

#include "dmxparams.h"
#include "h3/dmxsendmulti.h"
#include "storedmxsend.h"
#include "rdmdeviceparams.h"
#include "storerdmdevice.h"

#include "spiflashinstall.h"
#include "spiflashstore.h"
//
#include "remoteconfig.h"
#include "remoteconfigparams.h"
#include "storeremoteconfig.h"

#include "firmwareversion.h"

#include "software_version.h"

extern "C" {

void notmain(void) {
	Hardware hw;
	NetworkH3emac nw;
	LedBlink lb;
	DisplayUdf display;
	DisplayUdfHandler displayUdfHandler;
	FirmwareVersion fw(SOFTWARE_VERSION, __DATE__, __TIME__);

	SpiFlashInstall spiFlashInstall;
	SpiFlashStore spiFlashStore;

	StoreDmxSend storeDmxSend;
	StoreRDMDevice storeRdmDevice;

	ArtNet4Params artnetParams((ArtNet4ParamsStore *)spiFlashStore.GetStoreArtNet4());

	if (artnetParams.Load()) {
		artnetParams.Dump();
	}

	fw.Print();

	console_puts("Ethernet Art-Net 4 Node ");
	console_set_fg_color(CONSOLE_GREEN);
	console_puts("DMX Output");
	console_set_fg_color(CONSOLE_WHITE);
	console_puts(" / ");
	console_set_fg_color((artnetParams.IsRdm()) ? CONSOLE_GREEN : CONSOLE_WHITE);
	console_puts("RDM");
	console_set_fg_color(CONSOLE_WHITE);
#if defined(ORANGE_PI)
	console_puts(" {2 Universes}\n");
#else
	console_puts(" {4 Universes}\n");
#endif

	hw.SetLed(HARDWARE_LED_ON);

	console_status(CONSOLE_YELLOW, NetworkConst::MSG_NETWORK_INIT);
	display.TextStatus(NetworkConst::MSG_NETWORK_INIT, DISPLAY_7SEGMENT_MSG_INFO_NETWORK_INIT);

	nw.Init((NetworkParamsStore *)spiFlashStore.GetStoreNetwork());
	nw.SetNetworkStore((NetworkStore *)spiFlashStore.GetStoreNetwork());
	nw.SetNetworkDisplay((NetworkDisplay *)&displayUdfHandler);
	nw.Print();

	ArtNet4Node node;

	console_status(CONSOLE_YELLOW, ArtNetConst::MSG_NODE_PARAMS);
	display.TextStatus(ArtNetConst::MSG_NODE_PARAMS, DISPLAY_7SEGMENT_MSG_INFO_NODE_PARMAMS);

	artnetParams.Set(&node);

	uint8_t nAddress;
	bool bIsSetIndividual = false;
	bool bIsSet;

	nAddress = artnetParams.GetUniverse(0, bIsSet);

	if (bIsSet) {
		node.SetUniverseSwitch(0, ARTNET_OUTPUT_PORT, nAddress);
		bIsSetIndividual = true;
	}

	nAddress = artnetParams.GetUniverse(1, bIsSet);
	if (bIsSet) {
		node.SetUniverseSwitch(1, ARTNET_OUTPUT_PORT, nAddress);
		bIsSetIndividual = true;
	}
#if defined (ORANGE_PI_ONE)
	nAddress = artnetParams.GetUniverse(2, bIsSet);
	if (bIsSet) {
		node.SetUniverseSwitch(2, ARTNET_OUTPUT_PORT, nAddress);
		bIsSetIndividual = true;
	}
#ifndef DO_NOT_USE_UART0
	nAddress = artnetParams.GetUniverse(3, bIsSet);
	if (bIsSet) {
		node.SetUniverseSwitch(3, ARTNET_OUTPUT_PORT, nAddress);
		bIsSetIndividual = true;
	}
#endif
#endif

	if (!bIsSetIndividual) { // Backwards compatibility
		node.SetUniverseSwitch(0, ARTNET_OUTPUT_PORT, 0 + artnetParams.GetUniverse());
		node.SetUniverseSwitch(1, ARTNET_OUTPUT_PORT, 1 + artnetParams.GetUniverse());
#if defined (ORANGE_PI_ONE)
		node.SetUniverseSwitch(2, ARTNET_OUTPUT_PORT, 2 + artnetParams.GetUniverse());
#ifndef DO_NOT_USE_UART0
		node.SetUniverseSwitch(3, ARTNET_OUTPUT_PORT, 3 + artnetParams.GetUniverse());
#endif
#endif
	}

	DMXSendMulti dmxMulti;
	DMXParams dmxParams((DMXParamsStore *)&storeDmxSend);

	if (dmxParams.Load()) {
		dmxParams.Dump();
		dmxParams.Set(&dmxMulti);
	}

	node.SetOutput(&dmxMulti);
	node.SetDirectUpdate(false);

	IpProg ipprog;
	node.SetIpProgHandler(&ipprog);

	node.SetArtNetDisplay((ArtNetDisplay *)&displayUdfHandler);
	node.SetArtNetStore((ArtNetStore *)spiFlashStore.GetStoreArtNet());

	node.Print();
	dmxMulti.Print();

	ArtNetRdmController discovery;

	if(artnetParams.IsRdm()) {
		RDMDeviceParams rdmDeviceParams((RDMDeviceParamsStore *)&storeRdmDevice);

		if(rdmDeviceParams.Load()) {
			rdmDeviceParams.Set((RDMDevice *)&discovery);
			rdmDeviceParams.Dump();
		}

		discovery.Init();
		discovery.Print();

		if (artnetParams.IsRdmDiscovery()) {
			console_status(CONSOLE_YELLOW, ArtNetConst::MSG_RDM_RUN);
			display.TextStatus(ArtNetConst::MSG_RDM_RUN, DISPLAY_7SEGMENT_MSG_INFO_RDM_RUN);

			for (uint32_t i = 0; i < ARTNET_MAX_PORTS; i++) {
				uint8_t nAddress;
				if (node.GetUniverseSwitch(i, nAddress)) {
					discovery.Full(i);
				}
			}
		}

		node.SetRdmHandler((ArtNetRdm *)&discovery);
	}

	display.SetTitle("Eth Art-Net 4 %s", artnetParams.IsRdm() ? "RDM" : "DMX");
	display.Set(2, DISPLAY_UDF_LABEL_NODE_NAME);
	display.Set(3, DISPLAY_UDF_LABEL_IP);
	display.Set(4, DISPLAY_UDF_LABEL_VERSION);
	display.Set(5, DISPLAY_UDF_LABEL_UNIVERSE_PORT_A);
	display.Set(6, DISPLAY_UDF_LABEL_UNIVERSE_PORT_B);
	uint8_t nTextLength;
	display.Write(7, hw.GetBoardName(nTextLength));

	StoreDisplayUdf storeDisplayUdf;
	DisplayUdfParams displayUdfParams(&storeDisplayUdf);

	if(displayUdfParams.Load()) {
		displayUdfParams.Set(&display);
		displayUdfParams.Dump();
	}

	display.Show(&node);

	RemoteConfig remoteConfig(REMOTE_CONFIG_ARTNET, artnetParams.IsRdm() ? REMOTE_CONFIG_MODE_RDM : REMOTE_CONFIG_MODE_DMX, node.GetActiveOutputPorts());

	StoreRemoteConfig storeRemoteConfig;

	if (SpiFlashStore::Get()->HaveFlashChip()) {
		RemoteConfigParams remoteConfigParams(&storeRemoteConfig);

		if (remoteConfigParams.Load()) {
			remoteConfigParams.Set(&remoteConfig);
			remoteConfigParams.Dump();
		}
	} else {
		remoteConfig.SetDisable(true);
		printf("Remote configuration is disabled\n");
	}

	console_status(CONSOLE_YELLOW, ArtNetConst::MSG_NODE_START);
	display.TextStatus(ArtNetConst::MSG_NODE_START, DISPLAY_7SEGMENT_MSG_INFO_NODE_START);

	node.Start();

	console_status(CONSOLE_GREEN, ArtNetConst::MSG_NODE_STARTED);
	display.TextStatus(ArtNetConst::MSG_NODE_STARTED, DISPLAY_7SEGMENT_MSG_INFO_NODE_STARTED);

	hw.WatchdogInit();

	for (;;) {
		hw.WatchdogFeed();
		nw.Run();
		node.Run();
		remoteConfig.Run();
		spiFlashStore.Flash();
		lb.Run();
		display.Run();
	}
}

}
