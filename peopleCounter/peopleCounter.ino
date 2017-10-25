extern "C" {
  #include <user_interface.h>
}

#define DATA_LENGTH           112

#define TYPE_MANAGEMENT       0x00
#define TYPE_CONTROL          0x01
#define TYPE_DATA             0x02
#define SUBTYPE_PROBE_REQUEST 0x04

#define FRAME_TYPE_MGMT                 0
#define FRAME_TYPE_CTRL                 1
#define FRAME_TYPE_DATA                 2

#define FRAME_SUBTYPE_ASSOC_REQ         0
#define FRAME_SUBTYPE_ASSOC_RESP        1
#define FRAME_SUBTYPE_REASSOC_REQ       2
#define FRAME_SUBTYPE_REASSOC_RESP      3
#define FRAME_SUBTYPE_PROBE_REQ         4
#define FRAME_SUBTYPE_PROBE_RESP        5
#define FRAME_SUBTYPE_BEACON            8
#define FRAME_SUBTYPE_ATIM              9
#define FRAME_SUBTYPE_DISASSOC          10
#define FRAME_SUBTYPE_AUTH              11
#define FRAME_SUBTYPE_DEAUTH            12

#define MAC_LIMIT               700
#define CHANNEL_HOP_INTERVAL_MS (60000/14)

#define DEBUG_MAC               0
#define BINARY_OUTPUT           1

static os_timer_t channelHop_timer;
static uint8_t mac_list[MAC_LIMIT*6];
static uint16_t nmacs = 0;


struct RxControl
{
        signed rssi :8;
        unsigned rate :4;
        unsigned is_group :1;
        unsigned :1;
        unsigned sig_mode :2;
        unsigned legacy_length :12;
        unsigned damatch0 :1;
        unsigned damatch1 :1;
        unsigned bssidmatch0 :1;
        unsigned bssidmatch1 :1;
        unsigned MCS :7;
        unsigned CWB :1;
        unsigned HT_length :16;
        unsigned Smoothing :1;
        unsigned Not_Sounding :1;
        unsigned :1;
        unsigned Aggregation :1;
        unsigned STBC :2;
        unsigned FEC_CODING :1;
        unsigned SGI :1;
        unsigned rxend_state :8;
        unsigned ampdu_cnt :8;
        unsigned channel :4;
        unsigned :12;
}__attribute__((__packed__));

struct IEEE80211_Header
{
        struct
        {
                //buf[0]
                uint8_t Protocol :2;
                uint8_t Type :2;
                uint8_t Subtype :4;
                //buf[1]
                uint8_t ToDS :1;
                uint8_t FromDS :1;
                uint8_t MoreFlag :1;
                uint8_t Retry :1;
                uint8_t PwrMgmt :1;
                uint8_t MoreData :1;
                uint8_t Protectedframe :1;
                uint8_t Order :1;
        } frameControl;

        uint16_t duration;
        uint8_t address1[6];
        uint8_t address2[6];
        uint8_t address3[6];

        uint8_t fragmentNumber :4;
        uint16_t sequenceNumber :12;

        uint8_t address4[6];

        uint8_t qosControl[2];
        uint8_t htControl[4];
}__attribute__((__packed__));

/*Callback of promiscuous mode, it should decide if the package is worth keeping and set newReading
 * to force the main loop to actually parse it */
void promiscCallback(uint8* buf, uint16 len)
{

    /* If mac list is full no need to process package */
    if (nmacs >= MAC_LIMIT)
    {
      return;
    }
    struct RxControl* control = (struct RxControl*) buf;
    struct IEEE80211_Header* header = 0;
    if (len == 128)
    {
        // packet with header
        header = (struct IEEE80211_Header*) (buf + sizeof(struct RxControl));
    }
    else if (len % 10 == 0)
    {
        header = (struct IEEE80211_Header*) (buf + sizeof(struct RxControl));
    }
    else if (len == 12)
    {
        // No buf
        header = 0;
    }
    else
    {
        return; // Invalid length!
    }
    if (header)
    {
        uint8_t *dataBuff = (buf + sizeof(struct RxControl));
        // Ignore beacon frames
        if ((header->frameControl.Type == FRAME_TYPE_MGMT && header->frameControl.Subtype == FRAME_SUBTYPE_BEACON) || header->frameControl.Type == FRAME_SUBTYPE_PROBE_RESP || header->frameControl.Type == FRAME_TYPE_CTRL)
        {
            return;
        }
        // Limiting packet scan by type and subtype
        if ((header->frameControl.Type == FRAME_TYPE_MGMT && (header->frameControl.Subtype == FRAME_SUBTYPE_ASSOC_REQ || header->frameControl.Subtype == FRAME_SUBTYPE_REASSOC_REQ || header->frameControl.Subtype == FRAME_SUBTYPE_PROBE_REQ)) || header->frameControl.Type == FRAME_TYPE_DATA)
        {
            /* Maybe this should be done outside the interrupt, but for now going to keep it here */
            uint8_t mac[6];
            /* Each package has a diferent possition for the sender MAC address*/
            uint8_t macType = header->frameControl.ToDS << 1 | header->frameControl.FromDS;
            if (macType == 0 || macType == 2)
            {
                memcpy(mac, header->address2, 6);
            }
            else if (macType == 1)
            {
                memcpy(mac, header->address3, 6);
            }
            else
            {
                memcpy(mac, header->address4, 6);
            }

            /* Now lets check if the MAC is in the current list */
            uint16_t mac_counter;
            uint8_t found = 0;
            for(mac_counter = 0; mac_counter < nmacs; mac_counter++)
            {   
              /* memcmp returns 0 when arrays are equal */           
              if (memcmp(&mac_list[mac_counter*6], mac, 6) == 0)
              {
                found = 1;
                break;
              }
            }
            if (!found)
            {
              if(nmacs < MAC_LIMIT)
              {
#if DEBUG_MAC
                for(uint8_t i = 0; i < 6; i++)
                {
                  printf("%d,", mac[i]);
                }
                printf("\n");
#endif
                memcpy(&mac_list[nmacs*6], mac, 6);
                nmacs += 1;
              }
            }
        }
    }
}

/**
 * Callback for channel hoping
 */
void channelHop()
{
  // hoping channels 1-14
  uint8 new_channel = wifi_get_channel() + 1;
  if (new_channel > 14)
  {
    new_channel = 1;
#if BINARY_OUTPUT
    /*In order to facilitate reading going to print as 2 bytes*/
    uint8_t *nmacs_ptr = (uint8_t *)&nmacs;
    printf("%c%c", nmacs_ptr[0], nmacs_ptr[1]);
#else
    printf("%d\n", nmacs);
#endif
    nmacs = 0;
  }
  wifi_set_channel(new_channel);
}

void setup() {
  // set the WiFi chip to "promiscuous" mode aka monitor mode
  memset(mac_list, 0, MAC_LIMIT*6);
  Serial.begin(115200);
  delay(10);
  wifi_set_opmode(STATION_MODE);
  wifi_set_channel(1);
  wifi_promiscuous_enable(0);
  delay(10);
  wifi_set_promiscuous_rx_cb(promiscCallback);
  delay(10);
  wifi_promiscuous_enable(1);

  // setup the channel hoping callback timer
  os_timer_disarm(&channelHop_timer);
  os_timer_setfn(&channelHop_timer, (os_timer_func_t *) channelHop, NULL);
  os_timer_arm(&channelHop_timer, CHANNEL_HOP_INTERVAL_MS, 1);
}

void loop() {
  delay(1);
}
