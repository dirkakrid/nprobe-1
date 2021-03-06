/*
 * ssl.c
 * Copyright (C) 2009-2011 by ipoque GmbH
 *
 * This file is part of OpenDPI, an open source deep packet inspection
 * library based on the PACE technology by ipoque GmbH
 *
 * OpenDPI is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * OpenDPI is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with OpenDPI.  If not, see <http://www.gnu.org/licenses/>.
 *
 */


#include "ipq_utils.h"

#ifdef IPOQUE_PROTOCOL_SSL

#define IPOQUE_MAX_SSL_REQUEST_SIZE 10000

static void ipoque_int_ssl_add_connection(struct ipoque_detection_module_struct
					  *ipoque_struct, u32 protocol)
{
  if (protocol != IPOQUE_PROTOCOL_SSL) {
    ipoque_int_add_connection(ipoque_struct, protocol, IPOQUE_CORRELATED_PROTOCOL);
  } else {
    ipoque_int_add_connection(ipoque_struct, protocol, IPOQUE_REAL_PROTOCOL);
  }
}

#ifdef HAVE_NTOP
#ifndef WIN32
inline int min(int a, int b) { return(a < b ? a : b); }
#endif

static void stripCertificateTrailer(char *buffer, int buffer_len) {
  int i;

  for(i=0; i<buffer_len; i++) {
    if((buffer[i] != '.')
       && (buffer[i] != '-')
       && (!isalpha(buffer[i]))
       && (!isdigit(buffer[i])))
      buffer[i] = '\0';
    break;
  }
}

int getSSLcertificate(struct ipoque_detection_module_struct *ipoque_struct, char *buffer, int buffer_len) {
  struct ipoque_packet_struct *packet = &ipoque_struct->packet;

  /* Nothing matched so far: let's decode the certificate with some heuristics */
  if(packet->payload[0] == 0x16 /* Handshake */) {
    u_int16_t total_len  = packet->payload[4] + 5 /* SSL Header */;
    u_int8_t handshake_protocol = packet->payload[5];

    memset(buffer, 0, buffer_len);

    if(handshake_protocol == 0x02 /* Server Hello */) {
      int i;

      for(i=total_len; i < packet->payload_packet_len-3; i++) {
	if((packet->payload[i] == 0x04)
	   && (packet->payload[i+1] == 0x03)
	   && (packet->payload[i+2] == 0x0c)) {
	  u_int8_t server_len = packet->payload[i+3];

	  if(server_len+i+3 < packet->payload_packet_len) {
	    char *server_name = (char*)&packet->payload[i+4];
	    u_int8_t begin = 0, len, j, num_dots;

	    while(begin < server_len) {
	      if(!isprint(server_name[begin]))
		begin++;
	      else
		break;
	    }

	    len = min(server_len-begin, buffer_len-1);
	    strncpy(buffer, &server_name[begin], len);
	    buffer[len] = '\0';

	    /* We now have to check if this looks like an IP address or host name */
	    for(j=0, num_dots = 0; j<len; j++) {
	      if(!isprint((buffer[j]))) {
		num_dots = 0; /* This is not what we look for */
		break;
	      } else if(buffer[j] == '.') {
		num_dots++;
		if(num_dots >=2) break;
	      }
	    }

	    if(num_dots >= 2) {
	      stripCertificateTrailer(buffer, buffer_len);

	      return(1 /* Server Certificate */);
	    }
	  }
	}
      }
    } else if(handshake_protocol == 0x01 /* Client Hello */) {
      u_int offset, base_offset = 43;
      u_int16_t session_id_len = packet->payload[base_offset];
      if((session_id_len+base_offset+2) >= total_len) { 
	u_int16_t cypher_len =  packet->payload[session_id_len+base_offset+2];

	offset = base_offset + session_id_len + cypher_len + 2;

	if(offset < total_len) {
	  u_int16_t compression_len;
	  u_int16_t extensions_len;

	  compression_len = packet->payload[offset+1];
	  offset += compression_len + 3;
	  extensions_len = packet->payload[offset];

	  if((extensions_len+offset) < total_len) {
	    u_int16_t extension_offset = 1; /* Move to the first extension */

	    while(extension_offset < extensions_len) {
	      u_int16_t extension_id, extension_len;

	      memcpy(&extension_id, &packet->payload[offset+extension_offset], 2);
	      extension_offset += 2;

	      memcpy(&extension_len, &packet->payload[offset+extension_offset], 2);
	      extension_offset += 2;

	      extension_id = ntohs(extension_id), extension_len = ntohs(extension_len);

	      if(extension_id == 0) {
		u_int begin = 0,len;
		char *server_name = (char*)&packet->payload[offset+extension_offset];

		while(begin < extension_len) {
		  if((!isprint(server_name[begin]))
		     || ispunct(server_name[begin])
		     || isspace(server_name[begin]))
		    begin++;
		  else
		    break;
		}

		len = min(extension_len-begin, buffer_len-1);
		strncpy(buffer, &server_name[begin], len);
		buffer[len] = '\0';
		stripCertificateTrailer(buffer, buffer_len);

		/* We're happy now */
		return(2 /* Client Certificate */);
	      }

	      extension_offset += extension_len;
	    }
	  }
	}
      }
    }
  }

  return(0); /* Not found */
}

int sslDetectProtocolFromCertificate(struct ipoque_detection_module_struct *ipoque_struct) {
  struct ipoque_packet_struct *packet = &ipoque_struct->packet;

  if((packet->detected_protocol_stack[0] == IPOQUE_PROTOCOL_UNKNOWN)
     || (packet->detected_protocol_stack[0] == IPOQUE_PROTOCOL_SSL)) {
    char certificate[64];
    int rc = getSSLcertificate(ipoque_struct, certificate, sizeof(certificate));

    if(rc > 0) {
      /* printf("***** [SSL] %s\n", certificate); */
      matchStringProtocol(ipoque_struct, certificate, strlen(certificate));

      return(rc);
    }
  }

  return(0);
}

#endif

static void ssl_mark_and_payload_search_for_other_protocols(struct
							    ipoque_detection_module_struct
							    *ipoque_struct)
{
#if defined(IPOQUE_PROTOCOL_SOFTETHER) || defined(IPOQUE_PROTOCOL_MEEBO)|| defined(IPOQUE_PROTOCOL_TOR) || defined(IPOQUE_PROTOCOL_VPN_X) || defined(IPOQUE_PROTOCOL_UNENCRYPED_JABBER) || defined (IPOQUE_PROTOCOL_OOVOO) || defined (IPOQUE_PROTOCOL_ISKOOT) || defined (IPOQUE_PROTOCOL_OSCAR) || defined (IPOQUE_PROTOCOL_ITUNES) || defined (IPOQUE_PROTOCOL_GMAIL)

  struct ipoque_packet_struct *packet = &ipoque_struct->packet;
#ifdef IPOQUE_PROTOCOL_ISKOOT
  struct ipoque_flow_struct *flow = ipoque_struct->flow;
#endif
  //      struct ipoque_id_struct         *src=ipoque_struct->src;
  //      struct ipoque_id_struct         *dst=ipoque_struct->dst;
  u32 a;
  u32 end;
#if defined(IPOQUE_PROTOCOL_UNENCRYPED_JABBER)
  if (IPOQUE_COMPARE_PROTOCOL_TO_BITMASK(ipoque_struct->detection_bitmask, IPOQUE_PROTOCOL_UNENCRYPED_JABBER) != 0)
    goto check_for_ssl_payload;
#endif
#if defined(IPOQUE_PROTOCOL_OSCAR)
  if (IPOQUE_COMPARE_PROTOCOL_TO_BITMASK(ipoque_struct->detection_bitmask, IPOQUE_PROTOCOL_OSCAR) != 0)
    goto check_for_ssl_payload;
#endif
#if defined(IPOQUE_PROTOCOL_GADUGADU)
  if (IPOQUE_COMPARE_PROTOCOL_TO_BITMASK(ipoque_struct->detection_bitmask, IPOQUE_PROTOCOL_GADUGADU) != 0)
    goto check_for_ssl_payload;
#endif
  goto no_check_for_ssl_payload;

 check_for_ssl_payload:
  end = packet->payload_packet_len - 20;
  for (a = 5; a < end; a++) {
#ifdef IPOQUE_PROTOCOL_UNENCRYPED_JABBER
    if (packet->payload[a] == 't') {
      if (memcmp(&packet->payload[a], "talk.google.com", 15) == 0) {
	IPQ_LOG(IPOQUE_PROTOCOL_UNENCRYPED_JABBER, ipoque_struct, IPQ_LOG_DEBUG, "ssl jabber packet match\n");
	if (IPOQUE_COMPARE_PROTOCOL_TO_BITMASK
	    (ipoque_struct->detection_bitmask, IPOQUE_PROTOCOL_UNENCRYPED_JABBER) != 0) {
	  ipoque_int_ssl_add_connection(ipoque_struct, IPOQUE_PROTOCOL_UNENCRYPED_JABBER);
	  return;
	}
      }
    }
#endif
#ifdef IPOQUE_PROTOCOL_OSCAR
    if (packet->payload[a] == 'A' || packet->payload[a] == 'k' || packet->payload[a] == 'c'
	|| packet->payload[a] == 'h') {
      if (((a + 19) < packet->payload_packet_len && memcmp(&packet->payload[a], "America Online Inc.", 19) == 0)
	  //                        || (end - c > 3 memcmp (&packet->payload[c],"AOL", 3) == 0 )
	  //                        || (end - c > 7 && memcmp (&packet->payload[c], "AOL LLC", 7) == 0)
	  || ((a + 15) < packet->payload_packet_len && memcmp(&packet->payload[a], "kdc.uas.aol.com", 15) == 0)
	  || ((a + 14) < packet->payload_packet_len && memcmp(&packet->payload[a], "corehc@aol.net", 14) == 0)
	  || ((a + 41) < packet->payload_packet_len
	      && memcmp(&packet->payload[a], "http://crl.aol.com/AOLMSPKI/aolServerCert", 41) == 0)
	  || ((a + 28) < packet->payload_packet_len
	      && memcmp(&packet->payload[a], "http://ocsp.web.aol.com/ocsp", 28) == 0)
	  || ((a + 32) < packet->payload_packet_len
	      && memcmp(&packet->payload[a], "http://pki-info.aol.com/AOLMSPKI", 32) == 0)) {
	IPQ_LOG(IPOQUE_PROTOCOL_OSCAR, ipoque_struct, IPQ_LOG_DEBUG, "OSCAR SERVER SSL DETECTED\n");

	if (ipoque_struct->dst != NULL && packet->payload_packet_len > 75) {
	  memcpy(ipoque_struct->dst->oscar_ssl_session_id, &packet->payload[44], 32);
	  ipoque_struct->dst->oscar_ssl_session_id[32] = '\0';
	  ipoque_struct->dst->oscar_last_safe_access_time = packet->tick_timestamp;
	}

	ipoque_int_ssl_add_connection(ipoque_struct, IPOQUE_PROTOCOL_OSCAR);
	return;
      }
    }

    if (packet->payload[a] == 'm' || packet->payload[a] == 's') {
      if ((a + 21) < packet->payload_packet_len &&
	  (memcmp(&packet->payload[a], "my.screenname.aol.com", 21) == 0
	   || memcmp(&packet->payload[a], "sns-static.aolcdn.com", 21) == 0)) {
	IPQ_LOG(IPOQUE_PROTOCOL_OSCAR, ipoque_struct, IPQ_LOG_DEBUG, "OSCAR SERVER SSL DETECTED\n");
	ipoque_int_ssl_add_connection(ipoque_struct, IPOQUE_PROTOCOL_OSCAR);
	return;
      }
    }
#endif



  }
 no_check_for_ssl_payload:
#endif
  IPQ_LOG(IPOQUE_PROTOCOL_SSL, ipoque_struct, IPQ_LOG_DEBUG, "found ssl connection.\n");

#ifdef HAVE_NTOP
  sslDetectProtocolFromCertificate(ipoque_struct);

  if((packet->detected_protocol_stack[0] == IPOQUE_PROTOCOL_UNKNOWN)
     || (packet->detected_protocol_stack[0] == IPOQUE_PROTOCOL_SSL)) {
     /*
       Citrix GotoMeeting (AS16815, AS21866)
       216.115.208.0/20
       216.219.112.0/20
    */

    /* printf("[SSL] %08X / %08X\n", ntohl(packet->iph->saddr) , ntohl(packet->iph->daddr)); */

    if(((ntohl(packet->iph->saddr) & 0xFFFFF000 /* 255.255.240.0 */) == 0xD873D000 /* 216.115.208.0 */)
       || ((ntohl(packet->iph->daddr) & 0xFFFFF000 /* 255.255.240.0 */) == 0xD873D000 /* 216.115.208.0 */)

       || ((ntohl(packet->iph->saddr) & 0xFFFFF000 /* 255.255.240.0 */) == 0xD8DB7000 /* 216.219.112.0 */)
       || ((ntohl(packet->iph->daddr) & 0xFFFFF000 /* 255.255.240.0 */) == 0xD8DB7000 /* 216.219.112.0 */)
       ) {
      ipoque_int_add_connection(ipoque_struct, NTOP_PROTOCOL_CITRIX_ONLINE, IPOQUE_REAL_PROTOCOL);
      return;
    }

    /*
       Apple (FaceTime, iMessage,...)
       17.0.0.0/8
    */
    if(((ntohl(packet->iph->saddr) & 0xFF000000 /* 255.0.0.0 */) == 0x11000000 /* 17.0.0.0 */)
       || ((ntohl(packet->iph->daddr) & 0xFF000000 /* 255.0.0.0 */) == 0x11000000 /* 17.0.0.0 */)) {
      ipoque_int_add_connection(ipoque_struct, NTOP_PROTOCOL_APPLE, IPOQUE_REAL_PROTOCOL);
      return;
    }

    /*
      Webex
      66.114.160.0/20
    */
    if(((ntohl(packet->iph->saddr) & 0xFFFFF000 /* 255.255.240.0 */) == 0x4272A000 /* 66.114.160.0 */)
       || ((ntohl(packet->iph->daddr) & 0xFFFFF000 /* 255.255.240.0 */) ==0x4272A000 /* 66.114.160.0 */)) {
      ipoque_int_add_connection(ipoque_struct, NTOP_PROTOCOL_WEBEX, IPOQUE_REAL_PROTOCOL);
      return;
    }

    /*
      Google
      173.194.0.0/16
    */
    if(((ntohl(packet->iph->saddr) & 0xFFFF0000 /* 255.255.0.0 */) == 0xADC20000 /* 66.114.160.0 */)
       || ((ntohl(packet->iph->daddr) & 0xFFFF0000 /* 255.255.0.0 */) ==0xDC20000 /* 66.114.160.0 */)) {
      ipoque_int_add_connection(ipoque_struct, NTOP_PROTOCOL_GOOGLE, IPOQUE_REAL_PROTOCOL);
      return;
    }
  }
#endif

  ipoque_int_ssl_add_connection(ipoque_struct, IPOQUE_PROTOCOL_SSL);
}


static u8 ipoque_search_sslv3_direction1(struct ipoque_detection_module_struct *ipoque_struct)
{

  struct ipoque_packet_struct *packet = &ipoque_struct->packet;
  //  struct ipoque_flow_struct *flow = ipoque_struct->flow;
  //      struct ipoque_id_struct         *src=ipoque_struct->src;
  //      struct ipoque_id_struct         *dst=ipoque_struct->dst;


  if (packet->payload_packet_len >= 5 && packet->payload[0] == 0x16 && packet->payload[1] == 0x03
      && (packet->payload[2] == 0x00 || packet->payload[2] == 0x01 || packet->payload[2] == 0x02)) {
    u32 temp;
    IPQ_LOG(IPOQUE_PROTOCOL_SSL, ipoque_struct, IPQ_LOG_DEBUG, "search sslv3\n");
    // SSLv3 Record
    if (packet->payload_packet_len >= 1300) {
      return 1;
    }
    temp = ntohs(get_u16(packet->payload, 3)) + 5;
    IPQ_LOG(IPOQUE_PROTOCOL_SSL, ipoque_struct, IPQ_LOG_DEBUG, "temp = %u.\n", temp);
    if (packet->payload_packet_len == temp
	|| (temp < packet->payload_packet_len && packet->payload_packet_len > 500)) {
      return 1;
    }

    if (packet->payload_packet_len < temp && temp < 5000 && packet->payload_packet_len > 9) {
      /* the server hello may be split into small packets */
      u32 cert_start;

      IPQ_LOG(IPOQUE_PROTOCOL_SSL, ipoque_struct, IPQ_LOG_DEBUG,
	      "maybe SSLv3 server hello split into smaller packets\n");

      /* lets hope at least the server hello and the start of the certificate block are in the first packet */
      cert_start = ntohs(get_u16(packet->payload, 7)) + 5 + 4;
      IPQ_LOG(IPOQUE_PROTOCOL_SSL, ipoque_struct, IPQ_LOG_DEBUG, "suspected start of certificate: %u\n",
	      cert_start);

      if (cert_start < packet->payload_packet_len && packet->payload[cert_start] == 0x0b) {
	IPQ_LOG(IPOQUE_PROTOCOL_SSL, ipoque_struct, IPQ_LOG_DEBUG,
		"found 0x0b at suspected start of certificate block\n");
	return 2;
      }
    }

    if ((packet->payload_packet_len > temp && packet->payload_packet_len > 100) && packet->payload_packet_len > 9) {
      /* the server hello may be split into small packets and the certificate has its own SSL Record
       * so temp contains only the length for the first ServerHello block */
      u32 cert_start;

      IPQ_LOG(IPOQUE_PROTOCOL_SSL, ipoque_struct, IPQ_LOG_DEBUG,
	      "maybe SSLv3 server hello split into smaller packets but with seperate record for the certificate\n");

      /* lets hope at least the server hello record and the start of the certificate record are in the first packet */
      cert_start = ntohs(get_u16(packet->payload, 7)) + 5 + 5 + 4;
      IPQ_LOG(IPOQUE_PROTOCOL_SSL, ipoque_struct, IPQ_LOG_DEBUG, "suspected start of certificate: %u\n",
	      cert_start);

      if (cert_start < packet->payload_packet_len && packet->payload[cert_start] == 0x0b) {
	IPQ_LOG(IPOQUE_PROTOCOL_SSL, ipoque_struct, IPQ_LOG_DEBUG,
		"found 0x0b at suspected start of certificate block\n");
	return 2;
      }
    }


    if (packet->payload_packet_len >= temp + 5 && (packet->payload[temp] == 0x14 || packet->payload[temp] == 0x16)
	&& packet->payload[temp + 1] == 0x03) {
      u32 temp2 = ntohs(get_u16(packet->payload, temp + 3)) + 5;
      if (temp + temp2 > IPOQUE_MAX_SSL_REQUEST_SIZE) {
	return 1;
      }
      temp += temp2;
      IPQ_LOG(IPOQUE_PROTOCOL_SSL, ipoque_struct, IPQ_LOG_DEBUG, "temp = %u.\n", temp);
      if (packet->payload_packet_len == temp) {
	return 1;
      }
      if (packet->payload_packet_len >= temp + 5 &&
	  packet->payload[temp] == 0x16 && packet->payload[temp + 1] == 0x03) {
	temp2 = ntohs(get_u16(packet->payload, temp + 3)) + 5;
	if (temp + temp2 > IPOQUE_MAX_SSL_REQUEST_SIZE) {
	  return 1;
	}
	temp += temp2;
	IPQ_LOG(IPOQUE_PROTOCOL_SSL, ipoque_struct, IPQ_LOG_DEBUG, "temp = %u.\n", temp);
	if (packet->payload_packet_len == temp) {
	  return 1;
	}
	if (packet->payload_packet_len >= temp + 5 &&
	    packet->payload[temp] == 0x16 && packet->payload[temp + 1] == 0x03) {
	  temp2 = ntohs(get_u16(packet->payload, temp + 3)) + 5;
	  if (temp + temp2 > IPOQUE_MAX_SSL_REQUEST_SIZE) {
	    return 1;
	  }
	  temp += temp2;
	  IPQ_LOG(IPOQUE_PROTOCOL_SSL, ipoque_struct, IPQ_LOG_DEBUG, "temp = %u.\n", temp);
	  if (temp == packet->payload_packet_len) {
	    return 1;
	  }
	}

      }


    }

  }
  return 0;

}

void ipoque_search_ssl_tcp(struct ipoque_detection_module_struct *ipoque_struct)
{
  struct ipoque_packet_struct *packet = &ipoque_struct->packet;
  struct ipoque_flow_struct *flow = ipoque_struct->flow;
  //      struct ipoque_id_struct         *src=ipoque_struct->src;
  //      struct ipoque_id_struct         *dst=ipoque_struct->dst;

  u8 ret;

  if (packet->detected_protocol_stack[0] == IPOQUE_PROTOCOL_SSL) {
    if (flow->l4.tcp.ssl_stage == 3 && packet->payload_packet_len > 20 && flow->packet_counter < 5) {
      /* this should only happen, when we detected SSL with a packet that had parts of the certificate in subsequent packets
       * so go on checking for certificate patterns for a couple more packets
       */
      IPQ_LOG(IPOQUE_PROTOCOL_SSL, ipoque_struct, IPQ_LOG_DEBUG,
	      "ssl flow but check another packet for patterns\n");
      ssl_mark_and_payload_search_for_other_protocols(ipoque_struct);
      if (packet->detected_protocol_stack[0] == IPOQUE_PROTOCOL_SSL) {
	/* still ssl so check another packet */
	return;
      } else {
	/* protocol has changed so we are done */
	return;
      }
    }
    return;
  }

  IPQ_LOG(IPOQUE_PROTOCOL_SSL, ipoque_struct, IPQ_LOG_DEBUG, "search ssl\n");

#ifdef HAVE_NTOP
  {
    /* Check if this is whatsapp first (this proto runs over port 443) */
    char whatsapp_pattern[] = { 0x57, 0x41, 0x01, 0x01, 0x00 };

    if((packet->payload_packet_len > 5)
       && (memcmp(packet->payload, whatsapp_pattern, sizeof(whatsapp_pattern)) == 0)) {
      ipoque_int_add_connection(ipoque_struct, NTOP_PROTOCOL_WHATSAPP, IPOQUE_REAL_PROTOCOL);
      return;
    } else {
      /* No whatsapp, let's try SSL */
      if(sslDetectProtocolFromCertificate(ipoque_struct) > 0)
	return;
    }
  }
#endif

  if (packet->payload_packet_len > 40 && flow->l4.tcp.ssl_stage == 0) {
    IPQ_LOG(IPOQUE_PROTOCOL_SSL, ipoque_struct, IPQ_LOG_DEBUG, "first ssl packet\n");
    // SSLv2 Record
    if (packet->payload[2] == 0x01 && packet->payload[3] == 0x03
	&& (packet->payload[4] == 0x00 || packet->payload[4] == 0x01 || packet->payload[4] == 0x02)
	&& (packet->payload_packet_len - packet->payload[1] == 2)) {
      IPQ_LOG(IPOQUE_PROTOCOL_SSL, ipoque_struct, IPQ_LOG_DEBUG, "sslv2 len match\n");
      flow->l4.tcp.ssl_stage = 1 + packet->packet_direction;
      return;
    }

    if (packet->payload[0] == 0x16 && packet->payload[1] == 0x03
	&& (packet->payload[2] == 0x00 || packet->payload[2] == 0x01 || packet->payload[2] == 0x02)
	&& (packet->payload_packet_len - ntohs(get_u16(packet->payload, 3)) == 5)) {
      // SSLv3 Record
      IPQ_LOG(IPOQUE_PROTOCOL_SSL, ipoque_struct, IPQ_LOG_DEBUG, "sslv3 len match\n");
      flow->l4.tcp.ssl_stage = 1 + packet->packet_direction;
      return;
    }
  }

  if (packet->payload_packet_len > 40 &&
      flow->l4.tcp.ssl_stage == 1 + packet->packet_direction
      && flow->packet_direction_counter[packet->packet_direction] < 5) {
    return;
  }

  if (packet->payload_packet_len > 40 && flow->l4.tcp.ssl_stage == 2 - packet->packet_direction) {
    IPQ_LOG(IPOQUE_PROTOCOL_SSL, ipoque_struct, IPQ_LOG_DEBUG, "second ssl packet\n");
    // SSLv2 Record
    if (packet->payload[2] == 0x01 && packet->payload[3] == 0x03
	&& (packet->payload[4] == 0x00 || packet->payload[4] == 0x01 || packet->payload[4] == 0x02)
	&& (packet->payload_packet_len - 2) >= packet->payload[1]) {
      IPQ_LOG(IPOQUE_PROTOCOL_SSL, ipoque_struct, IPQ_LOG_DEBUG, "sslv2 server len match\n");
      ssl_mark_and_payload_search_for_other_protocols(ipoque_struct);
      return;
    }

    ret = ipoque_search_sslv3_direction1(ipoque_struct);
    if (ret == 1) {
      IPQ_LOG(IPOQUE_PROTOCOL_SSL, ipoque_struct, IPQ_LOG_DEBUG, "sslv3 server len match\n");
      ssl_mark_and_payload_search_for_other_protocols(ipoque_struct);
      return;
    } else if (ret == 2) {
      IPQ_LOG(IPOQUE_PROTOCOL_SSL, ipoque_struct, IPQ_LOG_DEBUG,
	      "sslv3 server len match with split packet -> check some more packets for SSL patterns\n");
      ssl_mark_and_payload_search_for_other_protocols(ipoque_struct);
      if (packet->detected_protocol_stack[0] == IPOQUE_PROTOCOL_SSL) {
	flow->l4.tcp.ssl_stage = 3;
      }
      return;
    }

    if (packet->payload_packet_len > 40 && flow->packet_direction_counter[packet->packet_direction] < 5) {
      IPQ_LOG(IPOQUE_PROTOCOL_SSL, ipoque_struct, IPQ_LOG_DEBUG, "need next packet\n");
      return;
    }
  }

  IPQ_LOG(IPOQUE_PROTOCOL_SSL, ipoque_struct, IPQ_LOG_DEBUG, "exclude ssl\n");
  IPOQUE_ADD_PROTOCOL_TO_BITMASK(flow->excluded_protocol_bitmask, IPOQUE_PROTOCOL_SSL);
  return;
}
#endif
