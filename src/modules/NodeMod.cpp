#include "NodeMod.h"
#include "../mesh/generated/meshtastic/node_mod.pb.h"
#include "Default.h"
#include "MeshService.h"
#include "NodeDB.h"
#include "PowerFSM.h"
#include "RTC.h"
#include "RadioLibInterface.h"
#include "Router.h"
#include "configuration.h"
#include "main.h"
#include <OLEDDisplay.h>
#include <OLEDDisplayUi.h>
#include <meshUtils.h>

NodeModModule *nodeModModule;

int32_t NodeModModule::runOnce()
{
    refreshUptime();
    sendToPhone();
    sendToMesh(false);
    return 120 * 1000;
}

bool NodeModModule::handleReceivedProtobuf(const meshtastic_MeshPacket &mp, meshtastic_NodeMod *t)
{
    return false; // Let others look at this message also if they want
}

meshtastic_MeshPacket *NodeModModule::allocReply()
{
    return NULL;
}

void NodeModModule::adminChangedStatus(){
    refreshUptime();
    sendToPhone();
    sendToMesh(true);
}

meshtastic_MeshPacket* NodeModModule::preparePacket(){
    meshtastic_NodeMod nodemod = meshtastic_NodeMod_init_zero;
    strncpy(nodemod.text_status, moduleConfig.nodemod.text_status, sizeof(nodemod.text_status));

    if(strlen(moduleConfig.nodemod.emoji) != 0){
        nodemod.has_emoji=true;
        strncpy(nodemod.emoji, moduleConfig.nodemod.emoji, sizeof(nodemod.emoji));
    }

    meshtastic_MeshPacket *p = allocDataProtobuf(nodemod);
    p->to = NODENUM_BROADCAST;
    p->decoded.want_response = false;
    p->priority = meshtastic_MeshPacket_Priority_BACKGROUND;
    return p;
}

void NodeModModule::sendToPhone()
{
    if ((lastSentStatsToPhone == 0) || (uptimeLastMs - lastSentStatsToPhone >= 360*1000)) {
        meshtastic_MeshPacket* packet = preparePacket();
        service->sendToPhone(packet);    
        lastSentStatsToPhone = uptimeLastMs;
    }
}
 
void NodeModModule::sendToMesh(bool statusChanged)
{
    bool allowedByDefault = ((lastSentToMesh == 0) || ((uptimeLastMs - lastSentToMesh) >= (1800 * 1000) && airTime->isTxAllowedAirUtil() && airTime->isTxAllowedChannelUtil(false)));
    bool allowedDueToStatusChange = (statusChanged == true && (uptimeLastMs - lastSentToMesh) >= (300 * 1000) && airTime->isTxAllowedAirUtil() && airTime->isTxAllowedChannelUtil(false));

    if (allowedByDefault || allowedDueToStatusChange)
    {
        if (strlen(moduleConfig.nodemod.text_status) == 0 && strlen(moduleConfig.nodemod.emoji) == 0) {
            return;
        }

        meshtastic_MeshPacket* packet = preparePacket();
        packet->priority = meshtastic_MeshPacket_Priority_BACKGROUND;
        service->sendToMesh(packet, RX_SRC_LOCAL, true);
        lastSentToMesh = uptimeLastMs;
    }
}