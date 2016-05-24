/*
 *        nProbe - a Netflow v5/v9/IPFIX probe for IPv4/v6
 *
 *       Copyright (C) 2002-12 Luca Deri <deri@ntop.org>
 *
 *                     http://www.ntop.org/
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "nprobe.h"
#include "createdata.h"
#ifdef WIN32
#define MSG_DONTWAIT 0
#endif

/* ****************************************************** */

static int exportBucketToNetflowV5(FlowHashBucket *myBucket,
				   FlowDirection direction) {

  if(direction == src2dst_direction /* src -> dst */) {
    if(myBucket->core.tuple.flowCounters.pktSent == 0) return(0); /* Nothing to export */

    readWriteGlobals->theV5Flow.flowRecord[readWriteGlobals->numFlows].input     = myBucket->ext ? htons(myBucket->ext->if_input) : 0;
    readWriteGlobals->theV5Flow.flowRecord[readWriteGlobals->numFlows].output    = myBucket->ext ? htons(myBucket->ext->if_output) : 0;
    readWriteGlobals->theV5Flow.flowRecord[readWriteGlobals->numFlows].srcaddr   = htonl(myBucket->core.tuple.src.ipType.ipv4);
    readWriteGlobals->theV5Flow.flowRecord[readWriteGlobals->numFlows].dstaddr   = htonl(myBucket->core.tuple.dst.ipType.ipv4);
    readWriteGlobals->theV5Flow.flowRecord[readWriteGlobals->numFlows].dPkts     = htonl(myBucket->core.tuple.flowCounters.pktSent);
    readWriteGlobals->theV5Flow.flowRecord[readWriteGlobals->numFlows].dOctets   = htonl(myBucket->core.tuple.flowCounters.bytesSent);
    readWriteGlobals->theV5Flow.flowRecord[readWriteGlobals->numFlows].first     = htonl(msTimeDiff(&myBucket->core.tuple.flowTimers.firstSeenSent,
												    &readOnlyGlobals.initialSniffTime));
    readWriteGlobals->theV5Flow.flowRecord[readWriteGlobals->numFlows].last      = htonl(msTimeDiff(&myBucket->core.tuple.flowTimers.lastSeenSent,
												    &readOnlyGlobals.initialSniffTime));
    readWriteGlobals->theV5Flow.flowRecord[readWriteGlobals->numFlows].srcport   = htons(myBucket->core.tuple.sport);
    readWriteGlobals->theV5Flow.flowRecord[readWriteGlobals->numFlows].dstport   = htons(myBucket->core.tuple.dport);
    readWriteGlobals->theV5Flow.flowRecord[readWriteGlobals->numFlows].tos       = myBucket->ext ? myBucket->ext->src2dstTos : 0;
    readWriteGlobals->theV5Flow.flowRecord[readWriteGlobals->numFlows].src_as    = myBucket->ext ? htons(getAS(&myBucket->core.tuple.src, &myBucket->ext->srcInfo)) : 0;
    readWriteGlobals->theV5Flow.flowRecord[readWriteGlobals->numFlows].dst_as    = myBucket->ext ? htons(getAS(&myBucket->core.tuple.dst, &myBucket->ext->dstInfo)) : 0;
    readWriteGlobals->theV5Flow.flowRecord[readWriteGlobals->numFlows].src_mask  = myBucket->ext ? ip2mask(&myBucket->core.tuple.src, &myBucket->ext->srcInfo) : 0;
    readWriteGlobals->theV5Flow.flowRecord[readWriteGlobals->numFlows].dst_mask  = myBucket->ext ? ip2mask(&myBucket->core.tuple.dst, &myBucket->ext->dstInfo) : 0;
    readWriteGlobals->theV5Flow.flowRecord[readWriteGlobals->numFlows].tcp_flags = myBucket->ext ? (u_int8_t)myBucket->ext->protoCounters.tcp.src2dstTcpFlags : 0;

    readWriteGlobals->flowExportStats.totExportedFlowBytes += myBucket->core.tuple.flowCounters.bytesSent;
    readWriteGlobals->flowExportStats.totExportedFlowPkts  += myBucket->core.tuple.flowCounters.pktSent;
  } else {
    if(myBucket->core.tuple.flowCounters.pktRcvd == 0) return(0); /* Nothing to export */

    readWriteGlobals->theV5Flow.flowRecord[readWriteGlobals->numFlows].input     = myBucket->ext ? htons(myBucket->ext->if_output) : 0;
    readWriteGlobals->theV5Flow.flowRecord[readWriteGlobals->numFlows].output    = myBucket->ext ? htons(myBucket->ext->if_input) : 0;
    readWriteGlobals->theV5Flow.flowRecord[readWriteGlobals->numFlows].srcaddr   = htonl(myBucket->core.tuple.dst.ipType.ipv4);
    readWriteGlobals->theV5Flow.flowRecord[readWriteGlobals->numFlows].dstaddr   = htonl(myBucket->core.tuple.src.ipType.ipv4);
    readWriteGlobals->theV5Flow.flowRecord[readWriteGlobals->numFlows].dPkts     = htonl(myBucket->core.tuple.flowCounters.pktRcvd);
    readWriteGlobals->theV5Flow.flowRecord[readWriteGlobals->numFlows].dOctets   = htonl(myBucket->core.tuple.flowCounters.bytesRcvd);
    readWriteGlobals->theV5Flow.flowRecord[readWriteGlobals->numFlows].first     = htonl(msTimeDiff(&myBucket->core.tuple.flowTimers.firstSeenRcvd,
												    &readOnlyGlobals.initialSniffTime));
    readWriteGlobals->theV5Flow.flowRecord[readWriteGlobals->numFlows].last      = htonl(msTimeDiff(&myBucket->core.tuple.flowTimers.lastSeenRcvd,
												    &readOnlyGlobals.initialSniffTime));
    readWriteGlobals->theV5Flow.flowRecord[readWriteGlobals->numFlows].srcport   = htons(myBucket->core.tuple.dport);
    readWriteGlobals->theV5Flow.flowRecord[readWriteGlobals->numFlows].dstport   = htons(myBucket->core.tuple.sport);
    readWriteGlobals->theV5Flow.flowRecord[readWriteGlobals->numFlows].tos       = myBucket->ext ? myBucket->ext->dst2srcTos : 0;
    readWriteGlobals->theV5Flow.flowRecord[readWriteGlobals->numFlows].src_as    = myBucket->ext ? htons(getAS(&myBucket->core.tuple.dst, &myBucket->ext->dstInfo)) : 0;
    readWriteGlobals->theV5Flow.flowRecord[readWriteGlobals->numFlows].dst_as    = myBucket->ext ? htons(getAS(&myBucket->core.tuple.src, &myBucket->ext->srcInfo)) : 0;
    readWriteGlobals->theV5Flow.flowRecord[readWriteGlobals->numFlows].src_mask  = myBucket->ext ? ip2mask(&myBucket->core.tuple.dst, &myBucket->ext->dstInfo) : 0;
    readWriteGlobals->theV5Flow.flowRecord[readWriteGlobals->numFlows].dst_mask  = myBucket->ext ? ip2mask(&myBucket->core.tuple.src, &myBucket->ext->srcInfo) : 0;
    readWriteGlobals->theV5Flow.flowRecord[readWriteGlobals->numFlows].tcp_flags = myBucket->ext ? (u_int8_t)myBucket->ext->protoCounters.tcp.dst2srcTcpFlags : 0;

    readWriteGlobals->flowExportStats.totExportedFlowBytes += myBucket->core.tuple.flowCounters.bytesRcvd;
    readWriteGlobals->flowExportStats.totExportedFlowPkts  += myBucket->core.tuple.flowCounters.pktRcvd;
  }

  readWriteGlobals->theV5Flow.flowRecord[readWriteGlobals->numFlows].proto = (u_int8_t)myBucket->core.tuple.proto;
  readWriteGlobals->flowExportStats.totExportedFlows++;

  //save_hbase(readWriteGlobals->numFlows,(ntohl(readWriteGlobals->theV5Flow.flowRecord[readWriteGlobals->numFlows].first) / 1000) + readOnlyGlobals.initialSniffTime.tv_sec);
  //traceEvent(TRACE_NORMAL,"num = %d",readWriteGlobals->numFlows);
#ifdef HAVE_MYSQL
  if(db_initialized) {
    char sql[2048];
    unsigned int first, last;

    first = (ntohl(readWriteGlobals->theV5Flow.flowRecord[readWriteGlobals->numFlows].first) / 1000) + readOnlyGlobals.initialSniffTime.tv_sec;
    last  = (ntohl(readWriteGlobals->theV5Flow.flowRecord[readWriteGlobals->numFlows].last) / 1000) + readOnlyGlobals.initialSniffTime.tv_sec;

    // traceEvent(TRACE_ERROR, "====> %u / %u [num_collectors=%u]", first, last, readOnlyGlobals.numCollectors);

    /* When you change DEFAULT_V9_TEMPLATE please also update the variable below */
    snprintf(sql, sizeof(sql),
	     "INSERT DELAYED INTO `%sflows` (PROTOCOL, IPV4_SRC_ADDR, IPV4_DST_ADDR, INPUT_SNMP, OUTPUT_SNMP, IN_PKTS, "
	     "IN_BYTES, FIRST_SWITCHED, LAST_SWITCHED, L4_SRC_PORT, L4_DST_PORT, SRC_TOS, SRC_AS, DST_AS, TCP_FLAGS) "
	     "VALUES ('%u', '%u', '%u', '%u', '%u', '%u', '%u', '%u', '%u', '%u', '%u', '%u', '%u', '%u', '%u')",
	     get_db_table_prefix(),
	     readWriteGlobals->theV5Flow.flowRecord[readWriteGlobals->numFlows].proto,
	     ntohl(readWriteGlobals->theV5Flow.flowRecord[readWriteGlobals->numFlows].srcaddr),
	     ntohl(readWriteGlobals->theV5Flow.flowRecord[readWriteGlobals->numFlows].dstaddr),
	     ntohs(readWriteGlobals->theV5Flow.flowRecord[readWriteGlobals->numFlows].input),
	     ntohs(readWriteGlobals->theV5Flow.flowRecord[readWriteGlobals->numFlows].output),
	     ntohl(readWriteGlobals->theV5Flow.flowRecord[readWriteGlobals->numFlows].dPkts),
	     ntohl(readWriteGlobals->theV5Flow.flowRecord[readWriteGlobals->numFlows].dOctets),
	     first,
	     last,
	     ntohs(readWriteGlobals->theV5Flow.flowRecord[readWriteGlobals->numFlows].srcport),
	     ntohs(readWriteGlobals->theV5Flow.flowRecord[readWriteGlobals->numFlows].dstport),
	     readWriteGlobals->theV5Flow.flowRecord[readWriteGlobals->numFlows].tos,
	     ntohs(readWriteGlobals->theV5Flow.flowRecord[readWriteGlobals->numFlows].src_as),
	     ntohs(readWriteGlobals->theV5Flow.flowRecord[readWriteGlobals->numFlows].dst_as),
	     readWriteGlobals->theV5Flow.flowRecord[readWriteGlobals->numFlows].tcp_flags);

    exec_sql_query(sql, 1);
  }
#endif

#ifdef HAVE_FASTBIT
  /* We support only IPv4 on FastBit*/
  dump_flow2fastbit(readOnlyGlobals.templateBuffers[V4_TEMPLATE_INDEX].v9TemplateElementList,
		    (char*)&readWriteGlobals->theV5Flow.flowRecord[readWriteGlobals->numFlows],
		    sizeof(struct flow_ver5_rec));
#endif

  if(readOnlyGlobals.enableHttpPlugin
     || readOnlyGlobals.enableDnsPlugin
     || readOnlyGlobals.enableMySQLPlugin
     || readOnlyGlobals.enableSipPlugin
     || readOnlyGlobals.enableOraclePlugin
     || readOnlyGlobals.enableGtpPlugin
     || readOnlyGlobals.enableRadiusPlugin
     || readOnlyGlobals.enableL7BridgePlugin
     ) {
    /* Dummy: used to dump flows on disk */
    checkPluginExport(NULL, direction, myBucket, NULL, NULL, NULL);
  }
  return(1);
}

/* ****************************************************** */

static int exportBucketToNetflowV9(FlowHashBucket *myBucket,
				   FlowDirection direction) {
  u_int flowBufBegin, flowBufMax, templateIndex;
  int numElements;
  u_int8_t isV4Flow;
#if defined(HAVE_MYSQL) || defined(HAVE_FASTBIT)
  char *the_buffer;
  u_int the_len;
#endif
  PluginInformation *head = myBucket->ext->plugin;

  if(readOnlyGlobals.dontSentBidirectionalV9Flows) {
    if(((myBucket->ext->swap_flow == 0) && (direction == dst2src_direction))
       || ((myBucket->ext->swap_flow == 1) && (direction == src2dst_direction)))
      return(0);
  }

  if((direction == dst2src_direction) && readOnlyGlobals.dontSentBidirectionalV9Flows)
    return(0);

  isV4Flow = ((myBucket->core.tuple.src.ipVersion == 4)
	      || (readOnlyGlobals.templateBuffers[V6_TEMPLATE_INDEX].v9TemplateElementList[0] == NULL))
    ? 1 : 0;

  templateIndex = isV4Flow ? V4_TEMPLATE_INDEX : V6_TEMPLATE_INDEX;
  flowBufMax = NETFLOW_MAX_BUFFER_LEN;

  if(direction == src2dst_direction /* src -> dst */) {
    if(myBucket->core.tuple.flowCounters.pktSent == 0) return(0); /* Nothing to export */

    readWriteGlobals->flowExportStats.totExportedFlowBytes += myBucket->core.tuple.flowCounters.bytesSent;
    readWriteGlobals->flowExportStats.totExportedFlowPkts  += myBucket->core.tuple.flowCounters.pktSent;
  } else {
    if(myBucket->core.tuple.flowCounters.pktRcvd == 0) return(0); /* Nothing to export */

    readWriteGlobals->flowExportStats.totExportedFlowBytes += myBucket->core.tuple.flowCounters.bytesRcvd;
    readWriteGlobals->flowExportStats.totExportedFlowPkts  += myBucket->core.tuple.flowCounters.pktRcvd;
  }

  readWriteGlobals->flowExportStats.totExportedFlows++;

  /*
     templateIndex is the default template to use but in case there is a
     better match we ought to use it
  */

  while(head) {
    if(head->plugin_used) {
      templateIndex = isV4Flow ? head->pluginPtr->v4TemplateIdx : head->pluginPtr->v6TemplateIdx;
      /* 1 match is enough ! */
      break;
    } else
      head = head->next;
  }

  flowBufBegin = readOnlyGlobals.templateBuffers[templateIndex].bufferLen;

  if(readOnlyGlobals.enable_debug)
    traceEvent(TRACE_INFO, "--->>> To dump flow [templateIndex=%u][tot=%u][max=%u]",
	       templateIndex, flowBufBegin, readOnlyGlobals.templateBuffers[templateIndex].bufferLen);

  flowPrintf(readOnlyGlobals.templateBuffers[templateIndex].v9TemplateElementList,
	     isV4Flow ? 1 /* IPv4 */ : 0 /* IPv6 */,
	     readOnlyGlobals.templateBuffers[templateIndex].buffer,
	     &flowBufBegin, &flowBufMax,
	     &numElements, 0, myBucket, direction, 0, 0);

#if defined(HAVE_MYSQL) || defined(HAVE_FASTBIT)
  the_buffer = &readOnlyGlobals.templateBuffers[templateIndex].buffer[readOnlyGlobals.templateBuffers[templateIndex].bufferLen];
  the_len = flowBufBegin - readOnlyGlobals.templateBuffers[templateIndex].bufferLen;

  if(readOnlyGlobals.enable_debug)
    traceEvent(TRACE_INFO, "--->>> Dumped flow [templateIndex=%u][the_len=%u][tot=%u][max=%u]",
	       templateIndex, the_len, flowBufBegin, readOnlyGlobals.templateBuffers[templateIndex].bufferLen);
#endif

#ifdef HAVE_MYSQL
  if(readOnlyGlobals.enable_debug)
    traceEvent(TRACE_INFO, "Dumping data onto MySQL using template Id %u", templateIndex);

  dump_flow2db(readOnlyGlobals.templateBuffers[templateIndex].v9TemplateElementList, the_buffer, the_len);
#endif

#ifdef HAVE_FASTBIT
  if(isV4Flow) /* We currently support FastBit only for IPv4 */
    dump_flow2fastbit(readOnlyGlobals.templateBuffers[V4_TEMPLATE_INDEX].v9TemplateElementList,
		      the_buffer, the_len);
#endif

  readOnlyGlobals.templateBuffers[templateIndex].bufferLen = flowBufBegin;
  readWriteGlobals->queuedDataToExport += flowBufBegin;

  return(1);
}

/* ****************************************************** */

#define HAVE_PORT(p,q) ((myBucket->core.tuple.proto == q) && ((myBucket->core.tuple.sport == p) || (myBucket->core.tuple.dport == p)))

/* ****************************************************** */

int exportBucketToNetflow(FlowHashBucket *myBucket,
			  FlowDirection direction) {
  int rc = 0;

  if(direction == src2dst_direction) {
    if(myBucket->core.tuple.flowCounters.pktSent == 0)
      return(1);
  } else {
    if(myBucket->core.tuple.flowCounters.pktRcvd == 0)
      return(1);
  }

  fillASInfo(myBucket);

  if(readOnlyGlobals.numCollectors > 0) {
    if(readOnlyGlobals.netFlowVersion == 5) {
      if(myBucket->core.tuple.src.ipVersion == 4)
	rc = exportBucketToNetflowV5(myBucket, direction);
      else {
	static char msgPrinted = 0;

	if(!msgPrinted) {
	  traceEvent(TRACE_INFO, "Unable to export IPv6 flow using NetFlow v5. Dropped.");
	  msgPrinted = 1;
	}
      }
    } else
      rc = exportBucketToNetflowV9(myBucket, direction);
  } else
    rc = 1;

  if(rc) {
    if(readOnlyGlobals.traceMode == 2)
      printFlow(myBucket, direction);

    if(readOnlyGlobals.dumpFormat == binary_core_flow_format) {
      if(readWriteGlobals->flowFd)
	fwrite(&myBucket->core.tuple, 1,
	       sizeof(myBucket->core.tuple), readWriteGlobals->flowFd);
    } else
      if((readOnlyGlobals.dumpFormat != binary_format)
	 && (readOnlyGlobals.dumpFormat != binary_core_flow_format)
       && (readWriteGlobals->flowFd
#ifdef HAVE_SQLITE
	|| (readWriteGlobals->sqlite3Handler != NULL)
#endif
	)
       && (readOnlyGlobals.userTemplateBuffer.v9TemplateElementList[0] != NULL))
      flowFilePrintf(readOnlyGlobals.userTemplateBuffer.v9TemplateElementList,
		     readWriteGlobals->flowFd, myBucket, direction);

    readWriteGlobals->flowExportStats.totExportedFlows++, readWriteGlobals->numFlows++,
      readWriteGlobals->totFlows++, readWriteGlobals->totFlowsRate++,
      readWriteGlobals->totFlowsSinceLastExport++;

    checkNetFlowExport(0);
  }

  return(rc);
}

/* ****************************************************** */

void checkNetFlowExport(int forceExport) {
  int emitFlow, deltaFlows = 0, flowExpired, sendTemplate = 0, i;

  if(((readWriteGlobals->numFlows == 0)
      || (readOnlyGlobals.numCollectors == 0))
     && (readOnlyGlobals.dumpFormat != binary_format)) {
    readWriteGlobals->numFlows = 0; /*
				       Fake flow export so that everything works
				       but flows are not exported
				    */

    for(i=0; i<readOnlyGlobals.numActiveTemplates; i++)
      readOnlyGlobals.templateBuffers[i].bufferLen = 0;

    return;
  }

  /*
     We need to avoid that periodic flow export can interfere
     with checkNetFlowExport() called after that a flow has been exported
  */
  pthread_rwlock_wrlock(&readWriteGlobals->checkExportLock);

#ifdef DEBUG
  traceEvent(TRACE_ERROR, "====> [queuedDataToExport=%u][templateFlowSize=%u]",
	     readWriteGlobals->queuedDataToExport, readOnlyGlobals.templateFlowSize);
#endif

  if(((readOnlyGlobals.netFlowVersion == 9) || (readOnlyGlobals.netFlowVersion == 10))
     && (readOnlyGlobals.numCollectors > 1) && (!readOnlyGlobals.reflectorMode) /* Round-robin mode */
     && (readOnlyGlobals.packetsBeforeSendingTemplates == 0) /* It's time to send the template */
     ) {
    if(readOnlyGlobals.netFlowVersion == 9) {
      initNetFlowV9Header(&readWriteGlobals->theV9Header);
      readWriteGlobals->theV9Header.count = htons(3);
    } else
      initIPFIXHeader(&readWriteGlobals->theIPFIXHeader);

    sendNetFlowV9V10(0, 1, 1);

    readOnlyGlobals.packetsBeforeSendingTemplates = readOnlyGlobals.numCollectors*readOnlyGlobals.templatePacketsDelta;
  } else {
    if((readOnlyGlobals.netFlowVersion == 9 || readOnlyGlobals.netFlowVersion == 10)
       && (readOnlyGlobals.packetsBeforeSendingTemplates == 0))
      deltaFlows = readOnlyGlobals.templateFlowSize, sendTemplate = 1;
  }

  emitFlow = ((deltaFlows+readWriteGlobals->numFlows) >= readOnlyGlobals.minNumFlowsPerPacket)
    || (forceExport && readWriteGlobals->shutdownInProgress)
    || sendTemplate /* || (pcapFile != NULL) */;

  if(!emitFlow) {
    gettimeofday(&readWriteGlobals->actTime, NULL);

    flowExpired = readWriteGlobals->lastExportTime.tv_sec
      && (((time(NULL)-readWriteGlobals->lastExportTime.tv_sec) > readOnlyGlobals.sendTimeout)
	  || (readWriteGlobals->actTime.tv_sec > (readWriteGlobals->lastExportTime.tv_sec+readOnlyGlobals.sendTimeout)));
  }

  if(forceExport || emitFlow || flowExpired) {
    if(readOnlyGlobals.netFlowVersion == 5) {
      initNetFlowV5Header(&readWriteGlobals->theV5Flow);
      readWriteGlobals->theV5Flow.flowHeader.count = htons(readWriteGlobals->numFlows);
      sendNetFlowV5(&readWriteGlobals->theV5Flow, 0);
    } else {
      if(readOnlyGlobals.netFlowVersion == 9) {
	initNetFlowV9Header(&readWriteGlobals->theV9Header);
	readWriteGlobals->theV9Header.count = (deltaFlows > 0) ? htons(4) : htons(1);
      } else {
	initIPFIXHeader(&readWriteGlobals->theIPFIXHeader);
	// readWriteGlobals->theIPFIXHeader.len = 0; /* To be filled later */
      }

      sendNetFlowV9V10(0, (deltaFlows > 0) ? 1 : 0, 0);

      if(readOnlyGlobals.packetsBeforeSendingTemplates == 0)
	readOnlyGlobals.packetsBeforeSendingTemplates = readOnlyGlobals.templatePacketsDelta;
      else
	readOnlyGlobals.packetsBeforeSendingTemplates--;
    }

    readWriteGlobals->numFlows = 0;
    readWriteGlobals->lastExportTime.tv_sec = readWriteGlobals->actTime.tv_sec,
      readWriteGlobals->lastExportTime.tv_usec = readWriteGlobals->actTime.tv_usec;
  }

  if(readWriteGlobals->lastExportTime.tv_sec == 0) {
    readWriteGlobals->lastExportTime.tv_sec = readWriteGlobals->actTime.tv_sec,
      readWriteGlobals->lastExportTime.tv_usec = readWriteGlobals->actTime.tv_usec;
  }

  pthread_rwlock_unlock(&readWriteGlobals->checkExportLock);
}

/* ******************************************* */

static int send_buffer(int s, const void *msg, size_t len,
		       int flags, const struct sockaddr *to, socklen_t tolen) {

  if(is_locked_send())
    return(len); /* Emulate successful send */
  else {
    int rc;

    if(readOnlyGlobals.flowExportDelay == 0) flags |= MSG_DONTWAIT;
    rc = sendto(s, msg, len, flags, to, tolen);

    if(rc == -1) {
      // traceEvent(TRACE_WARNING, "sendto(len=%u) returned errno=%d", len, errno);

      if(errno == EAGAIN) {
	rc = sendto(s, msg, len, flags, to, tolen);
      }
    }

    return(rc);
  }
}

/* ****************************************************** */

#ifdef IP_HDRINCL

#define BUFFER_SIZE 1500

/*
 * Checksum routine for Internet Protocol family headers (C Version)
 *
 * Borrowed from DHCPd
 */

static u_int32_t in_cksum(unsigned char *buf,
			  unsigned nbytes, u_int32_t sum) {
  uint i;

  /* Checksum all the pairs of bytes first... */
  for (i = 0; i < (nbytes & ~1U); i += 2) {
    sum += (u_int16_t) ntohs(*((u_int16_t *)(buf + i)));
    /* Add carry. */
    if(sum > 0xFFFF)
      sum -= 0xFFFF;
  }

  /* If there's a single byte left over, checksum it, too.   Network
     byte order is big-endian, so the remaining byte is the high byte. */
  if(i < nbytes) {
#ifdef DEBUG_CHECKSUM_VERBOSE
    debug ("sum = %x", sum);
#endif
    sum += buf [i] << 8;
    /* Add carry. */
    if(sum > 0xFFFF)
      sum -= 0xFFFF;
  }

  return sum;
}

/* ******************************************* */

static u_int32_t wrapsum (u_int32_t sum) {
  sum = ~sum & 0xFFFF;
  return htons(sum);
}

/* ******************************************* */

static int send_raw_socket(int sock, const void *dataBuffer,
			   int dataBufferLen, struct sockaddr_in *dest) {
  if(is_locked_send())
    return(dataBufferLen); /* Emulate successful send */
  else {
    static int ipHdrId = 0;
    int rc;
    char buffer[BUFFER_SIZE];
    unsigned int buffer_size = BUFFER_SIZE, headerLen;
    struct ip_header *ip_header;
    struct udp_header *udp_header;

    ip_header = (struct ip_header*) buffer;
    ip_header->ihl = 5;
    ip_header->version = 4;
    ip_header->tos = 0;
    ip_header->tot_len = htons(buffer_size);
    ip_header->id = htons(ipHdrId++);
    ip_header->ttl = 64;
    ip_header->frag_off = htons(0);
    ip_header->protocol = IPPROTO_UDP;
    ip_header->check = wrapsum(in_cksum((unsigned char *)ip_header,
					sizeof(struct ip_header), 0));
    ip_header->daddr = dest->sin_addr.s_addr;
    ip_header->saddr =  readOnlyGlobals.sockIn.sin_addr.s_addr;

    udp_header = (struct udp_header*)(buffer + sizeof(struct ip_header));
    udp_header->source = readOnlyGlobals.sockIn.sin_port;
    udp_header->dest = dest->sin_port;
    udp_header->len = htons(sizeof(struct udp_header)+dataBufferLen);
    udp_header->check  = 0; /* It must be 0 to compute the checksum */

    headerLen = sizeof(struct ip_header)+sizeof(struct udp_header);
    if(dataBufferLen > (BUFFER_SIZE-headerLen))
      dataBufferLen = BUFFER_SIZE-headerLen-1;
    memcpy(&buffer[headerLen], dataBuffer, dataBufferLen);

    buffer_size = headerLen+dataBufferLen;
    ip_header->tot_len  = htons(buffer_size);

    /*
      http://www.cs.nyu.edu/courses/fall01/G22.2262-001/class11.htm
      http://www.ietf.org/rfc/rfc0761.txt
      http://www.ietf.org/rfc/rfc0768.txt
    */
    udp_header->check = wrapsum(in_cksum((unsigned char *)udp_header, sizeof(struct udphdr),
					 in_cksum((unsigned char *)dataBuffer, dataBufferLen,
						  in_cksum((unsigned char *)&ip_header->saddr,
							   2*sizeof(ip_header->saddr),
							   IPPROTO_UDP + ntohs(udp_header->len)))));
    rc = send_buffer(sock, buffer, buffer_size, 0,
		     (struct sockaddr*)dest,
		     sizeof(struct sockaddr_in));

    /*
      printf("buff %d [rc=%d][dataBufferLen=%d]\n",
      buffer_size, rc, dataBufferLen);
    */

    return(rc > 0 ? (rc-headerLen) : rc);
  }
}

#endif /* IP_HDRINCL */

/* ******************************************* */

#define MAX_LOCK_CHECK_FREQUENCY   10 /* sec */

int is_locked_send(void) {
  static u_char show_message = 1;
  static time_t last_check = 0;
  static int last_returned_value = 0;
  time_t now = time(NULL);

  /* Avoid checking the lock file too often */
  if((now-last_check) < MAX_LOCK_CHECK_FREQUENCY)
    return(last_returned_value);

  if(readOnlyGlobals.flowLockFile != NULL) {
    struct stat buf;

    last_check = now;
    /* The lock file exists so no flows will be sent */
    if(stat(readOnlyGlobals.flowLockFile, &buf) == 0) {
      if(show_message) {
	traceEvent(TRACE_WARNING,
		   "Lock file is present: no flows will be emitted.");
	show_message = 0;
      }
      return(last_returned_value = 1);
    }
  }

  show_message = 1;
  return(last_returned_value = 0); /* Not locked */
}

/* ****************************************************** */

void reopenSocket(CollectorAddress *collector) {
  int rc, sockopt = 1;

  traceEvent(TRACE_WARNING,
	     "Attempting to reopen the socket. Please wait....");

  close(collector->sockFd), collector->sockFd = -1;

  if(collector->transport == TRANSPORT_TCP)
    collector->sockFd = socket(AF_INET, SOCK_STREAM, 0);
#ifdef HAVE_SCTP
  else if(collector->transport == TRANSPORT_SCTP)
    collector->sockFd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
#endif

  if(collector->sockFd == -1) {
    traceEvent(TRACE_ERROR,
	       "Fatal error while creating socket (%s). Trying again later.",
	       strerror(errno));
    return;
  }

  setsockopt(collector->sockFd, SOL_SOCKET, SO_REUSEADDR,
	     (char *)&sockopt, sizeof(sockopt));

  if(collector->transport == TRANSPORT_TCP) {
    if(collector->isIPv6) {
	rc = connect(collector->sockFd,
		     (struct sockaddr *)&collector->u.v6Address,
		     sizeof(collector->u.v6Address));
      } else {
	rc = connect(collector->sockFd,
		     (struct sockaddr *)&collector->u.v4Address,
		     sizeof(struct sockaddr_in));
      }

    if(rc == -1) {
      char msg[256], buf[64];

      snprintf(msg, sizeof(msg),
	       "Connection failed with remote peer %s [%s]. "
	       "Trying again later.",
	       CollectorAddress2Str(collector, buf, sizeof(buf)),
	       strerror(errno));

      traceEvent(TRACE_ERROR, "%s", msg);
      dumpLogEvent(collector_connection_error, severity_error, msg);
    } else {
      /* Peer reconnected */
      char buf[64], msg[256];

      snprintf(msg, sizeof(msg),
	       "Succesfully reconnected with remote collector %s",
	       CollectorAddress2Str(collector, buf, sizeof(buf)));

      dumpLogEvent(collector_connected, severity_info, msg);

      /*
	NOTE
	When a peer is reconnected the template should be resent
	only to it. However in order to keep the code simple, the
	template is resent to everyone.
      */
      /* Force the probe to resend the template */
      readOnlyGlobals.packetsBeforeSendingTemplates = 0;
      sendNetFlowV9V10(0, 1, 1);
    }
  }

  collector->flowSequence = 0;
}

/* ****************************************************** */

static int sendFlowData(CollectorAddress *collector, char *buffer,
			int bufferLength, int sequenceIncrement) {
  int rc;
  u_int32_t flow_sequence;
  struct timeval now;

  if(readOnlyGlobals.demo_mode) {
    if(collector->flowSequence > MAX_DEMO_FLOWS) {
      static u_char msg_shown = 0;

      if(!msg_shown) {
	printf("************************************************************************\n");
	printf("* NOTE: You have reached the max demo %d flows export: no more exports *\n", MAX_DEMO_FLOWS);
	printf("* NOTE: no additional flows will be exported by this nProbe instance   *\n");
	printf("*************************************************************************\n\n");
	msg_shown = 1;
      }

      readOnlyGlobals.demo_expired = 1;
      return(0);
    }
  }

  if(readOnlyGlobals.enable_debug)
    traceEvent(TRACE_INFO, "Sending %d bytes packet", bufferLength);

  errno = 0;
  gettimeofday(&now, NULL);

#ifdef DEBUG
  traceEvent(TRACE_INFO, "sendFlowData: len=%d\n", bufferLength);
#endif

  /*
    We need to fill the sequence number according to the collector
    sequence.
  */

#if 0
  traceEvent(TRACE_INFO, "**** flowSequence=%d [%d]",
	     collector->flowSequence, readWriteGlobals->theIPFIXHeader.len);
#endif

  flow_sequence = htonl(collector->flowSequence);
  if(readOnlyGlobals.netFlowVersion == 5) {
    struct flow_ver5_hdr *h = (struct flow_ver5_hdr*)buffer;
    h->flow_sequence = flow_sequence; /* version+count+sysUptime+unix_secs */
  } else if(readOnlyGlobals.netFlowVersion == 9) {
    V9FlowHeader *h = (V9FlowHeader*)buffer;
    h->flow_sequence = flow_sequence; /* version+count+sysUptime+unix_secs */
  } else if(readOnlyGlobals.netFlowVersion == 10) {
    IPFIXFlowHeader *h = (IPFIXFlowHeader*)buffer;
    h->flow_sequence = flow_sequence; /* version+count+sysUptime+unix_secs */
  }

  if(readWriteGlobals->flowFd) {
    if(readOnlyGlobals.dumpFormat == binary_format) {
      int rc;

      fprintf(readWriteGlobals->flowFd, "%04d", bufferLength);
      rc = fwrite(buffer, 1, bufferLength, readWriteGlobals->flowFd);

      if(rc != bufferLength)
	traceEvent(TRACE_WARNING, "fwrite error: wrote %d, expected %d", rc, bufferLength);
    }
  }

  if((readOnlyGlobals.numCollectors == 0) || readOnlyGlobals.none_specified)
    return(bufferLength); /* Fake good send */

  /*
    This delay is used to slow down export rate as some
    collectors might not be able to catch up with nProbe
  */
  if(readOnlyGlobals.flowExportDelay > 0) {
#ifndef WIN32
    struct timespec timeout;
#endif
    u_int32_t msDiff;
    u_short canPause = 0;

    /*
      if -B packetFlowGroup is set, we'll set
      canPause if we've sent packetFlowGroup packets
      then we'll pause for readOnlyGlobals.flowExportDelay
    */
    if(readOnlyGlobals.packetFlowGroup > 0) {
      readWriteGlobals->packetSentCount++;

      if((readWriteGlobals->packetSentCount >= readOnlyGlobals.packetFlowGroup)
	 && (collector->lastExportTime.tv_sec > 0)) {
	if(readOnlyGlobals.traceMode == 2)
	  traceEvent(TRACE_INFO, "Pausing %d ms because we've sent %d packet(s)",
		     readOnlyGlobals.flowExportDelay, readWriteGlobals->packetSentCount);
	canPause = 1;
	readWriteGlobals->packetSentCount = 1;
      }
    }

    if(canPause) {
      msDiff = msTimeDiff(&now, &collector->lastExportTime);

#if defined(DEBUG)
      traceEvent(TRACE_WARNING, "====>>>>>>> Last flow was sent %d ms ago", msDiff);
#endif

      if(msDiff < readOnlyGlobals.flowExportDelay) {
	msDiff = readOnlyGlobals.flowExportDelay - msDiff;

#ifndef WIN32
	timeout.tv_sec = 0;
	timeout.tv_nsec = 1000000*msDiff;

	while((nanosleep(&timeout, &timeout) == -1) && (errno == EINTR))
	  ; /* Do nothing */
#else
	waitForNextEvent(msDiff);
#endif
      }
    }
  }

  if(collector->transport == TRANSPORT_TCP) {
    fd_set writemask;
    struct timeval wait_time;

    FD_ZERO(&writemask);
    FD_SET(collector->sockFd, &writemask);
    memset(&wait_time, 0, sizeof(wait_time));

    wait_time.tv_sec = 1; /* Do not complain if < 1 sec */

    rc = -1;

    if(select(collector->sockFd+1, NULL, &writemask, NULL, &wait_time) > 0) {
      if(FD_ISSET(collector->sockFd, &writemask)) {
	errno = 0;
	rc = send(collector->sockFd, buffer, bufferLength, MSG_DONTWAIT /* Non blocking */);
	/* traceEvent(TRACE_WARNING, "======> send() returned %d [errno=%d]", rc, errno); */
      }
    } else {
      /* traceEvent(TRACE_WARNING, "======> select() returned %d [errno=%d]", rc, errno); */
      errno = -1; /* timeout */
    }
  } else {
    if(!collector->isIPv6) {
#ifdef IP_HDRINCL
      if(collector->transport == TRANSPORT_UDP_RAW)
	rc = send_raw_socket(collector->sockFd, buffer, bufferLength,
			     &collector->u.v4Address);
      else
#endif
	rc = send_buffer(collector->sockFd, buffer, bufferLength,
			 0, (struct sockaddr *)&collector->u.v4Address,
			 sizeof(collector->u.v4Address));
    } else
      rc = send_buffer(collector->sockFd, buffer, bufferLength,
		       0, (struct sockaddr *)&collector->u.v6Address,
		       sizeof(collector->u.v6Address));
  }

  /*
    Note that on NetFlow v9 the sequence number is
    incremented per NetFlow packet sent and not per
    flow sent as for previous versions or in IPFIX.
  */
  if(readOnlyGlobals.netFlowVersion == 10)
    collector->flowSequence += readWriteGlobals->totFlowsSinceLastExport;
  else
  collector->flowSequence += sequenceIncrement;

  readWriteGlobals->totFlowsSinceLastExport = 0;

  if(readOnlyGlobals.flowExportDelay > 0)
    memcpy(&collector->lastExportTime, &now, sizeof(struct timeval));

  if((rc == -1)
     && ((errno == EPIPE /* Broken pipe */)
	 || (errno == -1 /* Timeout */))) {
    char msg[256], buf[64];

    snprintf(msg, sizeof(msg), "Collector %s on socket %d %s [errno=%d/%s]",
	     CollectorAddress2Str(collector, buf, sizeof(buf)),
	     collector->sockFd,
	     (errno == EPIPE) ? "disconnected" : "timed out: disconnecting it",
	     errno, strerror(errno));
    traceEvent(TRACE_WARNING, "%s", msg);

    dumpLogEvent((errno == EPIPE) ? collector_disconnected : collector_too_slow, severity_warning, msg);
    reopenSocket(collector);
  }

  if(rc == bufferLength) {
    /* Everything is ok */
    readWriteGlobals->flowExportStats.totExportedBytes += rc, readWriteGlobals->flowExportStats.totExportedPkts++;
  }

  return(rc);
}

/* ****************************************************** */

void sendNetFlow(void *buffer, u_int32_t bufferLength,
		 u_char lastFlow, int sequenceIncrement,
		 u_char broadcastToAllCollectors) {
  u_int32_t rc = 0;
  static u_short collectorId = 0;

#ifdef TIME_PROTECTION
  {
    struct tm expireDate;

#define EXPIRE_DAY    30
#define EXPIRE_MONTH  8
#define EXPIRE_YEAR   2005

    memset(&expireDate, 0, sizeof(expireDate));
    expireDate.tm_mday = EXPIRE_DAY;
    expireDate.tm_mon  = EXPIRE_MONTH-1;
    expireDate.tm_year = EXPIRE_YEAR-1900;

    if(time(NULL) > mktime(&expireDate)) {
      traceEvent(TRACE_ERROR, "Sorry: this copy of nProbe is expired.\n");
      exit(0);
    }
  }
#endif

#ifdef DEBUG
  traceEvent(TRACE_INFO, "==>> sendNetFlow(%d) [numCollectors=%d]",
	     bufferLength, readOnlyGlobals.numCollectors);
#endif

  if(((readOnlyGlobals.numCollectors == 0) || readOnlyGlobals.none_specified)
     && (readOnlyGlobals.dumpFormat != binary_format)
     && (readOnlyGlobals.dumpFormat != binary_core_flow_format))
    return;

  errno = 0;

  if(readOnlyGlobals.reflectorMode || broadcastToAllCollectors) {
    /* Send all the flows to all collectors */
    int i;

    for(i = 0; i<readOnlyGlobals.numCollectors; i++) {
      if(readWriteGlobals->shutdownInProgress) break;

      rc = sendFlowData(&readOnlyGlobals.netFlowDest[i],
			buffer, bufferLength,
			sequenceIncrement);

      if(rc != bufferLength) {
	static u_char msgSent = 0;

	if(!msgSent) {
	  char msg[256];

	  snprintf(msg, sizeof(msg), "Error while exporting flows (%s)", strerror(errno));
	  traceEvent(TRACE_WARNING, "%s", msg);
	  dumpLogEvent(flow_export_error, severity_error, msg);
	  msgSent = 1;
	}
      } else {
#ifdef DEBUG
	char addrbuf[INET6_ADDRSTRLEN];

	if(readOnlyGlobals.netFlowDest[i].isIP == 0)
	  traceEvent(TRACE_INFO, "Sent flow packet to %s",
		     inet_ntoa(readOnlyGlobals.netFlowDest[i].u.v4Address.sin_addr));
	else
	  traceEvent(TRACE_INFO, "Sent flow packet to [%s]",
		     inet_ntop(AF_INET6, (void *)&(readOnlyGlobals.netFlowDest[i].u.IPAddress.ip),
			       addrbuf, sizeof (addrbuf)));
#endif /* DEBUG */
      }
    }
  } else {
    /* Send flows to all collectors in round robin */
    rc = sendFlowData(&readOnlyGlobals.netFlowDest[collectorId], buffer,
		      bufferLength, sequenceIncrement);

    /* Switch to next collector */
    if(readOnlyGlobals.numCollectors > 0)
      collectorId = (collectorId + 1) % readOnlyGlobals.numCollectors;
  }

  if((rc != bufferLength)
     && (errno != 0)
     && (!readWriteGlobals->shutdownInProgress)) {
    static u_char msgSent = 0;

    if(!msgSent) {
      char msg[256];

      snprintf(msg, sizeof(msg), "Error while exporting flows (%s) [%u/%u]", strerror(errno), rc, bufferLength);
      traceEvent(TRACE_WARNING, "%s", msg);
      dumpLogEvent(flow_export_error, severity_error, msg);
      msgSent = 1;
    }
  }
}

/* ****************************************************** */

void sendNetFlowV5(NetFlow5Record *theV5Flow, u_char lastFlow) {
  int len;

  if(theV5Flow->flowHeader.count == 0) return;

  if(readOnlyGlobals.traceMode == 2)
    traceEvent(TRACE_INFO, "Sending %d flows (NetFlow v5 format)",
	       ntohs(theV5Flow->flowHeader.count));

  len = (ntohs(theV5Flow->flowHeader.count)*sizeof(struct flow_ver5_rec)
	 +sizeof(struct flow_ver5_hdr));

  sendNetFlow((char *)theV5Flow, len, lastFlow,
	      ntohs(theV5Flow->flowHeader.count), 0);
}

/* ****************************************************** */

void initNetFlowV5Header(NetFlow5Record *theV5Flow) {
  memset(&theV5Flow->flowHeader, 0, sizeof(theV5Flow->flowHeader));

  theV5Flow->flowHeader.version        = htons(5);
  theV5Flow->flowHeader.sysUptime      = htonl(msTimeDiff(&readWriteGlobals->actTime,
							  &readOnlyGlobals.initialSniffTime));
  theV5Flow->flowHeader.unix_secs      = htonl(readWriteGlobals->actTime.tv_sec);
  theV5Flow->flowHeader.unix_nsecs     = htonl(readWriteGlobals->actTime.tv_usec/1000);
  /* NOTE: theV5Flow->flowHeader.flow_sequence will be filled by sendFlowData */
  theV5Flow->flowHeader.engine_type    = (u_int8_t)readOnlyGlobals.engineType;
  theV5Flow->flowHeader.engine_id      = (u_int8_t)readOnlyGlobals.engineId;

  theV5Flow->flowHeader.sampleRate     = /* readOnlyGlobals.fakePktSampling ? 0 : */ htons(readOnlyGlobals.pktSampleRate-1);
}

/* ****************************************************** */

void initNetFlowV9Header(V9FlowHeader *v9Header) {
  memset(v9Header, 0, sizeof(V9FlowHeader));
  v9Header->version        = htons(readOnlyGlobals.netFlowVersion);
  v9Header->sysUptime      = htonl(msTimeDiff(&readWriteGlobals->actTime, &readOnlyGlobals.initialSniffTime));
  v9Header->unix_secs      = htonl(time(NULL));
  v9Header->sourceId       = htonl((readOnlyGlobals.engineType << 8) + readOnlyGlobals.engineId);
}

/* ****************************************************** */

void initIPFIXHeader(IPFIXFlowHeader *v10Header) {
  memset(v10Header, 0, sizeof(IPFIXFlowHeader));
  v10Header->version             = htons(readOnlyGlobals.netFlowVersion);
  v10Header->sysUptime           = htonl(readWriteGlobals->actTime.tv_sec);
  v10Header->observationDomainId = htonl((readOnlyGlobals.engineType << 8) + readOnlyGlobals.engineId);
}

/* ****************************************************** */

static int padding(int len) {
  int module = len % 4;

  if(module == 0)
    return(0);
  else
    return(4 - module);
}

/* ****************************************************** */

static int sendFlowset(u_int16_t flowset_id, char *flowBuffer, u_int flowBufferLen, int *bufLen) {
  int len, pad;
  V9FlowSet flowSet;

  len = readOnlyGlobals.templateBuffers[flowset_id].bufferLen;

  if(len == 0) return(0); /* No flows to send */

  flowSet.templateId = htons(readOnlyGlobals.idTemplate + flowset_id);
  len += 4;
  pad = padding(len); len += pad;
  flowSet.flowsetLen = htons(len);
  memcpy(&flowBuffer[(*bufLen)], &flowSet, sizeof(flowSet));
  (*bufLen) += sizeof(flowSet);

  if(((*bufLen)+readOnlyGlobals.templateBuffers[flowset_id].bufferLen) >= flowBufferLen) {
    static u_char warning_sent = 0;

    if(!warning_sent) {
      traceEvent(TRACE_WARNING,
		 "Internal error: too many NetFlow flows per packet (see -m) [%u/%u]",
		 ((*bufLen)+readOnlyGlobals.templateBuffers[flowset_id].bufferLen),
		 flowBufferLen);
      warning_sent = 1;
    }

    readOnlyGlobals.templateBuffers[flowset_id].bufferLen = flowBufferLen-(*bufLen)-1;
  }

  memcpy(&flowBuffer[(*bufLen)], readOnlyGlobals.templateBuffers[flowset_id].buffer, readOnlyGlobals.templateBuffers[flowset_id].bufferLen);
  (*bufLen) += readOnlyGlobals.templateBuffers[flowset_id].bufferLen;
  (*bufLen) += pad;

  return(1);
}

/* ****************************************************** */

void sendNetFlowV9V10(u_char lastFlow,
		      u_char sendTemplate,
		      u_char sendOnlyTheTemplate) {
  char flowBuffer[1514 /* Ethernet MTU */ - 42 /* Ethernet+IP+UDP header */] = { 0 };
  int bufLen = 0, len, pad, num_extra_elems = 0, i;

  /* traceEvent(TRACE_WARNING, "****** Sending templates... %d [%d]", sendTemplate, sendOnlyTheTemplate); */

  /*
    NOTE:
    In order to keep things simple, whenever there are multiple
    collectors in round robin and the template needs to be sent out
    it is sent alone (i.e. without incuding flows) to all the collectors.

    If there is just one collector, the template also contains flows
    up to the MTU size.
  */
  if(sendTemplate) {
    V9TemplateHeader templateHeader;
    V9TemplateDef templateDef;
    V9OptionTemplate optionTemplateDef;
    char tmpBuffer[256];
    u_int flowBufBegin, flowBufMax, i, maxTemplatePktLen = 1200, beginIdx = 0, endIdx = 0, numTemplatesSent = 0;
    int numElements, optionTemplateId = readOnlyGlobals.idTemplate + readOnlyGlobals.numActiveTemplates;
    V9FlowSet optionsFlowSet;

    while(numTemplatesSent < readOnlyGlobals.numActiveTemplates) {
      bufLen = 0, num_extra_elems = 0;

      /* NOTE: flow_sequence will be filled by sendFlowData */
      if(readOnlyGlobals.netFlowVersion == 9) {
	memcpy(&flowBuffer[bufLen], &readWriteGlobals->theV9Header, sizeof(readWriteGlobals->theV9Header));
	bufLen += sizeof(readWriteGlobals->theV9Header);
      } else {
	/* IPFIX */
	memcpy(&flowBuffer[bufLen], &readWriteGlobals->theIPFIXHeader, sizeof(readWriteGlobals->theIPFIXHeader));
	bufLen += sizeof(readWriteGlobals->theIPFIXHeader);
      }

      /* Header */
      num_extra_elems++;
      templateHeader.templateFlowset = (readOnlyGlobals.netFlowVersion == 9) ? htons(0) : htons(2); /* CHECK: is 2 is valid all the time ? */
      len = sizeof(V9TemplateHeader);

      for(i=beginIdx; i<readOnlyGlobals.numActiveTemplates; i++) {
	u_int to_add;

	to_add = sizeof(V9TemplateDef) + readOnlyGlobals.templateBuffers[i].templateBufBegin;

	if((len + to_add) > maxTemplatePktLen)
	  break;

	len += to_add;
	endIdx = i, numTemplatesSent++;
      }

      pad = padding(len); len += pad;
      templateHeader.flowsetLen = htons(len);
      memcpy(&flowBuffer[bufLen], &templateHeader, sizeof(V9TemplateHeader)); bufLen += sizeof(V9TemplateHeader);

      /* Dump templates */
      for(i=beginIdx; i <= endIdx; i++) {
	templateDef.fieldCount = htons(readOnlyGlobals.templateBuffers[i].numTemplateFieldElements);
	templateDef.templateId = htons(readOnlyGlobals.idTemplate+i);
	memcpy(&flowBuffer[bufLen], &templateDef, sizeof(V9TemplateDef));
	bufLen += sizeof(V9TemplateDef);
	memcpy(&flowBuffer[bufLen], readOnlyGlobals.templateBuffers[i].templateBuffer,
	       readOnlyGlobals.templateBuffers[i].templateBufBegin);
	bufLen += readOnlyGlobals.templateBuffers[i].templateBufBegin;
      }

      bufLen += pad; /* Add padding */

      /*
	Here is the relevant paragraph from rfc 5101 "3.4.2.1.  Scope".

	The scope is an Information Element specified in the IPFIX
	Information Model [RFC5102].  An IPFIX-compliant implementation of
	the Collecting Process SHOULD support this minimum set of Information
	Elements as scope: LineCardId, TemplateId, exporterIPv4Address,
	exporterIPv6Address, and ingressInterface.  Note that other
	Information Elements, such as meteringProcessId, exportingProcessId,
	observationDomainId, etc. are also valid scopes.  The IPFIX protocol
	doesn't prevent the use of any Information Elements for scope.
	However, some Information Element types don't make sense if specified
	as scope; for example, the counter Information Elements.

	Finally, note that the Scope Field Count MUST NOT be zero.

	The main things to note.
	1) supporting scope is a SHOULD not a MUST so not implementing it may be OK
	2) Counters (and I would argue totals) don't make sense as scope
	3) if you send an options template there MUST be at least one scope field

	Interim solution: we forget scope with IPFIX leaving it to a future nProbe release
      */

      if(numTemplatesSent == readOnlyGlobals.numActiveTemplates) {
	/* We send the scope only when we're done with templates */

	if(readOnlyGlobals.netFlowVersion == 9) {
	  u_int16_t vals[2] = { htons(1) /* Line Card */, htons(4) /* length = 2 bytes */};
	  u_int32_t system_ip = 0x0;

	  /* Options Template */
	  num_extra_elems++;
	  optionTemplateDef.templateFlowset = (readOnlyGlobals.netFlowVersion == 9) ? htons(1) : htons(3);
	  len = sizeof(V9OptionTemplate)+readOnlyGlobals.optionTemplateBufBegin+4;
	  pad = padding(len); len += pad;
	  optionTemplateDef.flowsetLen     = htons(len);
	  optionTemplateDef.templateId     = htons(optionTemplateId);
	  if(readOnlyGlobals.netFlowVersion == 9) {
	    optionTemplateDef.optionScopeLen = htons(4 /* SystemId=2 + SystemLen=2 */);
	    optionTemplateDef.optionLen      = htons(4 /* each field is 4 bytes */
						     * (readOnlyGlobals.numOptionTemplateFieldElements));
	  } else {
	    optionTemplateDef.optionScopeLen = htons(2 /* SystemId + SystemLen */);
	    optionTemplateDef.optionLen      = htons(readOnlyGlobals.numOptionTemplateFieldElements);
	  }

	  memcpy(&flowBuffer[bufLen], &optionTemplateDef, sizeof(V9OptionTemplate));
	  bufLen += sizeof(V9OptionTemplate);

	  /* Options */
	  memcpy(&flowBuffer[bufLen], vals, sizeof(vals));
	  bufLen += sizeof(vals);

	  memcpy(&flowBuffer[bufLen], readOnlyGlobals.optionTemplateBuffer,
		 readOnlyGlobals.optionTemplateBufBegin);
	  bufLen += readOnlyGlobals.optionTemplateBufBegin;
	  bufLen += pad;

	  /* Options DataRecord */
	  num_extra_elems++;
	  flowBufBegin = 0, flowBufMax = sizeof(tmpBuffer);
	  flowPrintf(readOnlyGlobals.v9OptionTemplateElementList,
		     1 /* IPv4 */, tmpBuffer, &flowBufBegin, &flowBufMax,
		     &numElements, 0, NULL, 0, 0, 1);

	  len = 4 /* sizeof(systemId) */ + flowBufBegin+sizeof(optionsFlowSet);
	  pad = padding(len); len += pad;
	  optionsFlowSet.templateId = htons(optionTemplateId);
	  optionsFlowSet.flowsetLen = htons(len);

	  memcpy(&flowBuffer[bufLen], &optionsFlowSet, sizeof(optionsFlowSet));
	  bufLen += sizeof(optionsFlowSet);

	  /* System IP */
	  memcpy(&flowBuffer[bufLen], &system_ip, flowBufBegin);
	  bufLen += sizeof(system_ip);

	  memcpy(&flowBuffer[bufLen], tmpBuffer, flowBufBegin);
	  bufLen += flowBufBegin;
	  bufLen += pad;
	} /* Scope */
      }

      if(readOnlyGlobals.netFlowVersion == 10) {
	/* Fill in the flow length */
	u_int16_t len = htons(bufLen);

	memcpy(&flowBuffer[2], &len, 2);
      }

#ifdef DEBUG
      traceEvent(TRACE_ERROR, "--->>> Sending %u bytes template packet", bufLen);
#endif

      sendNetFlow(&flowBuffer, bufLen, 0, 1, 1);
      beginIdx = endIdx+1;
    } /* while */
  }

  if(!sendOnlyTheTemplate) {
    u_int num, beginIdx, numTemplateFlowsSent = 0;

    while(numTemplateFlowsSent < readOnlyGlobals.numActiveTemplates) {
      bufLen = 0, num_extra_elems = 0, num = 0, beginIdx = 0;

      /* NOTE: flow_sequence will be filled by sendFlowData */
      if(readOnlyGlobals.netFlowVersion == 9) {
	memcpy(&flowBuffer[bufLen], &readWriteGlobals->theV9Header,
	       sizeof(readWriteGlobals->theV9Header));
	bufLen += sizeof(readWriteGlobals->theV9Header);
      } else {
	/* IPFIX */
	memcpy(&flowBuffer[bufLen], &readWriteGlobals->theIPFIXHeader,
	       sizeof(readWriteGlobals->theIPFIXHeader));
	bufLen += sizeof(readWriteGlobals->theIPFIXHeader);
      }

      /* Send all buffered data */
      for(i=beginIdx; i<readOnlyGlobals.numActiveTemplates; i++) {
	int n;

	n = sendFlowset(i, flowBuffer, sizeof(flowBuffer), &bufLen);
	num += n;

#ifdef DEBUG
	if(n > 0)
	  traceEvent(TRACE_ERROR, "--->>> Sending flowset %u/%u [id=%d][len=%u][bufLen=%u][num=%u]",
		     i, readOnlyGlobals.numActiveTemplates,
		     readOnlyGlobals.idTemplate + i, n, bufLen, num);
#endif

	numTemplateFlowsSent++;

	if(bufLen > 1300 /* bytes */)
	  break;
      }

      /* Fill in the flow length */
      if(readOnlyGlobals.netFlowVersion == 9) {
	u_int16_t len = htons(num+num_extra_elems);

	memcpy(&flowBuffer[2], &len, 2);
      } else if(readOnlyGlobals.netFlowVersion == 10) {
	u_int16_t len = htons(bufLen);

	memcpy(&flowBuffer[2], &len, 2);
      }

      sendNetFlow(flowBuffer, bufLen, 0, 1, 0);

#ifdef DEBUG
      traceEvent(TRACE_ERROR, "--->>> Sending %u bytes flow packet", bufLen);
#endif

      beginIdx = numTemplateFlowsSent+1;
    } /* while */
  }

  for(i=0; i<readOnlyGlobals.numActiveTemplates; i++)
    readOnlyGlobals.templateBuffers[i].bufferLen = 0;
}
