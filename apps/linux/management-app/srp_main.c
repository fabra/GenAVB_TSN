/*
 * Copyright 2018-2020 NXP.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *    Neither the name of NXP Semiconductors nor the
 *    names of its contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 @file
 @brief GenAVB SRP management application
 @details

 Copyright 2018-2020 NXP.
 All Rights Reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>

#include <genavb/genavb.h>
#include "../common/helpers.h"
#include "common.h"

//#define DEBUG

static char *module_name = "srp";
static char *device_name;

static int managed_get(struct genavb_control_handle *ctrl_h, void *cmd, unsigned int cmd_len, void *response, unsigned int response_len)
{
	unsigned int msg_type;
#ifdef DEBUG
	int i;
#endif
	int rc;

	if (!ctrl_h)
		goto err;

	msg_type = GENAVB_MSG_MANAGED_MSRP_GET;
	rc = genavb_control_send_sync(ctrl_h, &msg_type, cmd, cmd_len, response, &response_len, 100);
	if (rc != GENAVB_SUCCESS) {
		printf("genavb_control_send_sync() failed: %s\n", genavb_strerror(rc));
		goto err;
	}

	if (msg_type != GENAVB_MSG_MANAGED_MSRP_GET_RESPONSE) {
		printf("genavb_control_send_sync() wrong response message type: %d\n", msg_type);
		goto err;
	}

#ifdef DEBUG
	printf("%u\n", response_len);

	for (i = 0; i < response_len; i++)
		printf("%1x\n", ((uint8_t *)response)[i]);

	printf("\n");
#endif

	return 0;

err:
	return -1;
}

static int srp_bridge_base_table(struct genavb_control_handle *ctrl_h)
{
	struct genavb_msg_managed_get_response get_response;
	uint16_t cmd[10];
	uint16_t rep[6];
	uint8_t *data;
	uint16_t id, length, status;
	char name[][64] = {
		"Bridge Base Table",
		"msrpEnabledStatus",
		"talkerPruning",
		"msrpMaxFanInPorts",
		"msrpLatencyMaxFrameSize"
	};

	cmd[0] = 0; /* SRP_BRIDGE_BASE_TABLE */
	cmd[1] = 16;
	cmd[2] = 0; /* msrpEnabledStatus */
	cmd[3] = 0;
	cmd[4] = 1; /* talkerPruning */
	cmd[5] = 0;
	cmd[6] = 2; /* msrpMaxFanInPorts */
	cmd[7] = 0;
	cmd[8] = 3; /* msrpLatencyMaxFrameSize */
	cmd[9] = 0;

	if (managed_get(ctrl_h, cmd, 4 + cmd[1], &get_response, sizeof(get_response)) < 0)
		goto err;

	rep[0] = 0; /* bridge_base_table */
	rep[1] = 36;
	rep[2] = 0; /* status Ok */

	if (check_response(rep, (u_int16_t *)&get_response, 3) < 0)
		goto err;

	printf("%s %s %s\n", module_name, device_name, name[0]);

	data = get_response_header((uint8_t *)&get_response, &id, &length, &status);

	/* msrpEnabledStatus */
	data = get_node_header(data, &id, &length, &status);
	if (!status) {
		printf("%-40s %u\n", name[1], data[0]);
	}

	data = get_node_next(data, length);

	/* talkerPruning */
	data = get_node_header(data, &id, &length, &status);
	if (!status) {
		printf("%-40s %u\n", name[2], data[0]);
	}

	data = get_node_next(data, length);

	/* msrpMaxFanInPorts */
	data = get_node_header(data, &id, &length, &status);
	if (!status) {
		printf("%-40s %u\n", name[3], ((uint32_t *)data)[0]);
	}

	data = get_node_next(data, length);

	/* msrpLatencyMaxFrameSize */
	data = get_node_header(data, &id, &length, &status);
	if (!status) {
		printf("%-40s %u\n", name[4], ((uint32_t *)data)[0]);
	}

	return 0;
err:
	printf("%s %s %s: error\n", module_name, device_name, name[0]);

	return -1;
}

static int srp_bridge_port_table(struct genavb_control_handle *ctrl_h, unsigned int port)
{
	struct genavb_msg_managed_get_response get_response;
	uint16_t cmd[16];
	uint16_t rep[6];
	uint8_t *data;
	uint16_t id, length, status, total_length;
	char name[][64] = {
		"Bridge Port Table",
		"PortID",
		"msrpPortEnabledStatus",
		"FailedRegistrations",
		"LastPDUOrigin",
		"SR_PVID"
	};

	cmd[0] = 1; /* SRP_BRIDGE_PORT_TABLE  */
	cmd[1] = 22;
	cmd[2] = 0; /* 1st key leaf index, port number */
	cmd[3] = 2; /* 1st key length */
	cmd[4] = port; /* 1st key value */
	cmd[5] = 1; /* msrpPortEnabledStatus */
	cmd[6] = 0;
	cmd[7] = 2; /* FailedRegistrations */
	cmd[8] = 0;
	cmd[9] = 3; /* LastPDUOrigin */
	cmd[10] = 0;
	cmd[11] = 4; /* SR_PVID */
	cmd[12] = 0;

	if (managed_get(ctrl_h, cmd, 4 + cmd[1], &get_response, sizeof(get_response)) < 0)
		goto err;

	rep[0] = 1; /* bridge_port_table */
	rep[1] = 51;
	rep[2] = 0; /* status Ok*/
	rep[3] = 0; /* key index, port number */
	rep[4] = 4;
	rep[5] = 0; /* status ok */

	if (check_response(rep, (u_int16_t *)&get_response, 6) < 0)
		goto err;

	printf("%s %s %s\n", module_name, device_name, name[0]);

	/* table header (id, total length and status) */
	data = get_response_header((uint8_t *)&get_response, &id, &total_length, &status);

	/* PortID  */
	data = get_node_header(data, &id, &length, &status);
	if (!status) {
		printf("%-40s %u\n", name[1], ((uint16_t *)data)[0]);
	}

	data = get_node_next(data, length);

	/* msrpPortEnabledStatus */
	data = get_node_header(data, &id, &length, &status);
	if (!status) {
		printf("%-40s %u\n", name[2], data[0]);
	}

	data = get_node_next(data, length);

	/* FailedRegistrations */
	data = get_node_header(data, &id, &length, &status);
	if (!status) {
		printf("%-40s %" PRIu64 "\n", name[3], ((uint64_t *)data)[0]);
	}

	data = get_node_next(data, length);

	/* LastPDUOrigin */
	data = get_node_header(data, &id, &length, &status);
	if (!status) {
		printf("%-40s %02x:%02x:%02x:%02x:%02x:%02x\n", name[4], data[0], data[1], data[2], data[3], data[4], data[5]);
	}

	data = get_node_next(data, length);

	/* SR_PVID  */
	data = get_node_header(data, &id, &length, &status);
	if (!status) {
		printf("%-40s %u\n", name[5], ((uint16_t *)data)[0]);
	}

	return 0;
err:
	printf("%s %s %s: error\n", module_name, device_name, name[0]);

	return -1;

}

static int srp_streams_table(struct genavb_control_handle *ctrl_h, uint8_t *key_value, uint16_t key_length)
{
	struct genavb_msg_managed_get_response get_response;
	uint16_t cmd[32];
	uint16_t rep[6];
	uint8_t *data;
	int i, cmd_idx;
	uint16_t id, length, status, total_length;
	char name[][64] = {
		"Streams Table",
		"StreamID",
		"StreamDestinationAddress",
		"StreamVID",
		"MaxFrameSize",
		"MaxIntervalFrames",
		"DataFramePriority",
		"Rank",
	};

	cmd[0] = 3; /* SRP_STREAMS_TABLE */
	cmd[1] = 0;

	cmd[2] = 0; /* 1st key leaf index, StreamID */
	cmd[3] = key_length; /* 1st key length */
	cmd_idx = 4;
	cmd[1] += 4;

	if (key_length) {
		memcpy(&cmd[cmd_idx], key_value, key_length); /* 1st key value */
		cmd_idx += (key_length / sizeof(uint16_t));
		cmd[1] += key_length;
	}

	for (i = 0; i < 6 /* SRP_STREAM_NUM_LEAVES */; i++) {
		cmd[cmd_idx + 2 * i] = i + 1; /* stream parameter index */
		cmd[cmd_idx + 2 * i + 1] = 0;
		cmd[1] += 4;
	}

	if (managed_get(ctrl_h, cmd, 4 + cmd[1], &get_response, sizeof(get_response)) < 0)
		goto err;

	rep[0] = 3; /* srp_stream_table */
	rep[1] = 0; /* variable length */
	rep[2] = 0; /* status Ok*/
	rep[3] = 0; /* streamID key */

	if (check_response(&rep[2], &((u_int16_t *)&get_response)[2], 2) < 0)
		goto err;

	printf("%s %s %s\n", module_name, device_name, name[0]);

	/* table header (id, total length and status) */
	data = get_response_header((uint8_t *)&get_response, &id, &total_length, &status);

	while (data < ((uint8_t *)&get_response + (total_length + 4))) {
		/*  1st key (StreamID) */
		data = get_node_header(data, &id, &length, &status);
		if (!status) {
			printf("%-40s %016" PRIx64 "\n", name[1], ((uint64_t *)data)[0]);
		} else
			printf("%-40s failed %u\n", name[1], status);

		data = get_node_next(data, length);

		/* StreamDestinationAddress */
		data = get_node_header(data, &id, &length, &status);
		if (!status) {
			printf("%-40s %02x:%02x:%02x:%02x:%02x:%02x\n", name[2], data[0], data[1], data[2], data[3], data[4], data[5]);
		}

		data = get_node_next(data, length);

		/* StreamVID */
		data = get_node_header(data, &id, &length, &status);
		if (!status) {
			printf("%-40s %u\n", name[3], ((uint16_t *)data)[0]);
		}

		data = get_node_next(data, length);

		/* MaxFrameSize */
		data = get_node_header(data, &id, &length, &status);
		if (!status) {
			printf("%-40s %u\n", name[4], ((uint32_t *)data)[0]);
		}

		data = get_node_next(data, length);

		/* MaxIntervalFrames */
		data = get_node_header(data, &id, &length, &status);
		if (!status) {
			printf("%-40s %u\n", name[5], ((uint32_t *)data)[0]);
		}

		data = get_node_next(data, length);

		/* DataFramePriority */
		data = get_node_header(data, &id, &length, &status);
		if (!status) {
			printf("%-40s %u\n", name[6], ((uint32_t *)data)[0]);
		}

		data = get_node_next(data, length);

		/* Rank */
		data = get_node_header(data, &id, &length, &status);
		if (!status) {
			printf("%-40s %u\n", name[7], ((uint32_t *)data)[0]);
		}

		data = get_node_next(data, length);

		printf("\n");
	}

	return 0;
err:
	printf("%s %s %s: error\n", module_name, device_name, name[0]);

	return -1;
}


static int srp_reservations_table(struct genavb_control_handle *ctrl_h, unsigned int port, unsigned int direction, uint8_t *key_value, uint16_t key_length)
{
	struct genavb_msg_managed_get_response get_response;
	uint16_t cmd[64];
	uint16_t rep[6];
	uint8_t *data;
	int i, cmd_idx;
	uint16_t id, length, status, total_length;
	char name[][64] = {
		"Reservations Table",
		"PortID",
		"StreamID",
		"Direction",
		"DeclarationType",
		"AccumulatedLatency",
		"FailedBridgeId",
		"FailureCode",
		"DroppedFrames",
		"StreamAge",
	};

	cmd[0] = 4; /* SRP_RESERVATIONS_TABLE */
	cmd[1] = 0;

	cmd[2] = 0; /* 1st key index, port number */
	cmd[3] = 2; /* key length */
	cmd[4] = port; /* key value */
	cmd[1] += 6;

	cmd[5] = 1; /* 2nd key leaf, stream ID */
	cmd[6] = key_length; /* key length */
	cmd_idx = 7;
	cmd[1] += 4;

	if (key_length) {
		memcpy(&cmd[cmd_idx], key_value, key_length);
		cmd_idx += (key_length  / sizeof(uint16_t));
		cmd[1] += key_length;
	}

	cmd[cmd_idx] = 2; /* 3rd key index, direction  */
	cmd_idx++;
	cmd[cmd_idx] = 2; /* key length */
	cmd_idx++;
	cmd[cmd_idx] = direction; /* key value */
	cmd_idx++;
	cmd[1] += 6;

	for (i = 0; i < 6 /* SRP_RESERVATIONS_NUM_LEAVES */; i++) {
		cmd[cmd_idx + 2 * i] = i + 3; /* start at DeclarationType parameter index */
		cmd[cmd_idx + 2 * i + 1] = 0;
		cmd[1] += 4;
	}

	if (managed_get(ctrl_h, cmd, 4 + cmd[1], &get_response, sizeof(get_response)) < 0)
		goto err;

	rep[0] = 4; /* srp_reservation_table */
	rep[1] = 0; /* variable length */
	rep[2] = 0; /* status Ok*/
	rep[3] = 0; /* list entry */
	rep[4] = 4;
	rep[5] = 0; /* status ok */

	if (check_response(&rep[2], &((u_int16_t *)&get_response)[2], 4) < 0)
		goto err;

	printf("%s %s %s\n", module_name, device_name, name[0]);

	/* table header (id, total length and status) */
	data = get_response_header((uint8_t *)&get_response, &id, &total_length, &status);

	while (data < ((uint8_t *)&get_response + (total_length + 4))) {
		/* 1st key, PortID */
		data = get_node_header(data, &id, &length, &status);
		if (!status) {
			printf("%-40s %u\n", name[1], ((uint16_t *)data)[0]);
		}

		data = get_node_next(data, length);

		/* 2nd key, StreamID */
		data = get_node_header(data, &id, &length, &status);
		if (!status) {
			printf("%-40s %016" PRIx64 "\n", name[2], ((uint64_t *)data)[0]);
		}

		data = get_node_next(data, length);

		/* 3rd key, Direction */
		data = get_node_header(data, &id, &length, &status);
		if (!status) {
			printf("%-40s %u\n", name[3], ((uint16_t *)data)[0]);
		}

		data = get_node_next(data, length);

		/* DeclarationType */
		data = get_node_header(data, &id, &length, &status);
		if (!status) {
			printf("%-40s %u\n", name[4], ((uint32_t *)data)[0]);
		}

		data = get_node_next(data, length);

		/* AccumulatedLatency */
		data = get_node_header(data, &id, &length, &status);
		if (!status) {
			printf("%-40s %u\n", name[5], ((uint32_t *)data)[0]);
		}

		data = get_node_next(data, length);

		/* FailedBridgeId */
		data = get_node_header(data, &id, &length, &status);
		if (!status) {
			printf("%-40s %" PRIx64 "\n", name[6], ((uint64_t *)data)[0]);
		}

		data = get_node_next(data, length);

		/* FailureCode */
		data = get_node_header(data, &id, &length, &status);
		if (!status) {
			printf("%-40s %u\n", name[7], ((uint32_t *)data)[0]);
		}

		data = get_node_next(data, length);

		/* DroppedFrames */
		data = get_node_header(data, &id, &length, &status);
		if (!status) {
			printf("%-40s %" PRIu64 "\n", name[8], ((uint64_t *)data)[0]);
		}

		data = get_node_next(data, length);

		/* StreamAge */
		data = get_node_header(data, &id, &length, &status);
		if (!status) {
			printf("%-40s %u\n", name[9], ((uint32_t *)data)[0]);
		}

		data = get_node_next(data, length);

		printf("\n");
	}

	return 0;
err:
	printf("%s %s %s: error\n", module_name, device_name, name[0]);

	return -1;
}

static int srp_latency_parameter_table(struct genavb_control_handle *ctrl_h, unsigned int port, unsigned short tc_class)
{
	struct genavb_msg_managed_get_response get_response;
	uint16_t cmd[16];
	uint16_t rep[32];
	uint8_t *data;
	uint16_t id, length, status, total_length;
	char name[][64] = {
		"Latency Parameters Table",
		"PortID",
		"TrafficClass",
		"PortTcMaxLatency",
	};

	cmd[0] = 2; /* SRP_LATENCY_TABLE */
	cmd[1] = 0;

	cmd[2] = 0; /* key index, port number */
	cmd[3] = 2;
	cmd[4] = port; /* key value */
	cmd[1] += 6;

	cmd[5] = 1; /* key index, traffic class */
	cmd[6] = 4;
	cmd[7] = tc_class;
	cmd[8] = 0;
	cmd[1] += 8;

	cmd[9] = 2; /* latency value */
	cmd[10] = 0;
	cmd[1] += 4;

	if (managed_get(ctrl_h, cmd, 4 + cmd[1], &get_response, sizeof(get_response)) < 0)
		goto err;

	rep[0] = 2; /* srp_latency_table */
	rep[1] = 30;/* total length */
	rep[2] = 0; /* status ok */

	rep[3] = 0; /* port index */
	rep[4] = 4;
	rep[5] = 0; /* status ok */
	rep[6] = port; /*value */

	rep[7] = 1; /* traffic class index */
	rep[8] = 6;
	rep[9] = 0; /* status ok */
	rep[10] = tc_class; /* value */
	rep[11] = 0;

	if (check_response(rep, (u_int16_t *)&get_response, 12) < 0)
		goto err;

	printf("%s %s %s\n", module_name, device_name, name[0]);

	/* table header (id, total length and status) */
	data = get_response_header((uint8_t *)&get_response, &id, &total_length, &status);

	/* 1st key, PortID */
	data = get_node_header(data, &id, &length, &status);
	if (!status) {
		printf("%-40s %u\n", name[1], ((uint16_t *)data)[0]);
	}

	data = get_node_next(data, length);

	/* 2nd key, Traffic Class */
	data = get_node_header(data, &id, &length, &status);
	if (!status) {
		printf("%-40s %u\n", name[2], ((uint32_t *)data)[0]);
	}

	data = get_node_next(data, length);

	/* PortTcMaxLatency */
	data = get_node_header(data, &id, &length, &status);
	if (!status) {
		printf("%-40s %u\n", name[3], ((uint32_t *)data)[0]);
	}

	return 0;
err:
	printf("%s %s %s: error\n", module_name, device_name, name[0]);

	return -1;
}

int srp_main(struct genavb_handle *avb_h, int argc, char *argv[])
{
	struct genavb_control_handle *ctrl_h;
	struct genavb_control_handle *endpoint_msrp_ctrl_h;
	struct genavb_control_handle *bridge_msrp_ctrl_h;
	unsigned int port, direction;
	unsigned int set;
	unsigned long optval_ul;
	unsigned long long stream_key;
	unsigned long tc_key;
	unsigned int key_len;
	int option;
	int rc = 0;

	rc = genavb_control_open(avb_h, &endpoint_msrp_ctrl_h, GENAVB_CTRL_MSRP);
	if (rc != GENAVB_SUCCESS)
		endpoint_msrp_ctrl_h = NULL;

	rc = genavb_control_open(avb_h, &bridge_msrp_ctrl_h, GENAVB_CTRL_MSRP_BRIDGE);
	if (rc != GENAVB_SUCCESS)
		bridge_msrp_ctrl_h = NULL;

	if ((endpoint_msrp_ctrl_h == NULL) && (bridge_msrp_ctrl_h == NULL))
		goto err_control_open;

	/* default options */
	ctrl_h = endpoint_msrp_ctrl_h;
	device_name = "endpoint";
	port = 0;
	direction = 0;
	set = 0;

	while ((option = getopt(argc, argv, "EBGSP:D:s::r::pbl:h")) != -1) {

		/* common options */
		switch (option) {
		case 'E':
			ctrl_h = endpoint_msrp_ctrl_h;
			device_name = "endpoint";
			break;

		case 'B':
			ctrl_h = bridge_msrp_ctrl_h;
			device_name = "bridge";
			break;

		case 'G':
			set = 0;
			break;

		case 'S':
			set = 1;
			break;

		case 'P':
			if (h_strtoul(&optval_ul, optarg, NULL, 0) < 0) {
				usage();
				rc = -1;
				goto exit;
			}
			port = (unsigned int)optval_ul;
			break;

		case 'D':
			if (h_strtoul(&optval_ul, optarg, NULL, 0) < 0) {
				usage();
				rc = -1;
				goto exit;
			}
			direction = (unsigned int)optval_ul;
			break;

		case 's':
			/* streams table */
			if (set) {
				usage();
				rc = -1;
				goto exit;
			} else {
				/* stream ID */
				if (!argv[optind]) {
					key_len = 0;
				} else {
					key_len = sizeof(uint64_t);
					if (h_strtoull(&stream_key, argv[optind], NULL, 0) < 0) {
						usage();
						rc = -1;
						goto exit;
					}
				}
				rc = srp_streams_table(ctrl_h, (uint8_t *)&stream_key, key_len);
			}
			break;

		case 'r':
			/* reservation table */
			if (set) {
				usage();
				rc = -1;
				goto exit;
			} else {
				/* Stream ID */
				if (!argv[optind]) {
					key_len = 0;
				} else {
					key_len = sizeof(uint64_t);
					if (h_strtoull(&stream_key, argv[optind], NULL, 0) < 0) {
						usage();
						rc = -1;
						goto exit;
					}
				}
				rc = srp_reservations_table(ctrl_h, port, direction, (uint8_t *)&stream_key, key_len);
			}
			break;

		case 'b':
			/* bridge base table */
			if (set) {
				usage();
				rc = -1;
				goto exit;
			} else {
				rc = srp_bridge_base_table(ctrl_h);
			}
			break;

		case 'p':
			/* bridge ports table */
			if (set) {
				usage();
				rc = -1;
				goto exit;	
			} else {
				rc = srp_bridge_port_table(ctrl_h, port);
			}
			break;

		case 'l':
			/* latency parameters table */
			if (set) {
				usage();
				rc = -1;
				goto exit;
			} else {
				/* Traffic Class */
				if (h_strtoul(&tc_key, optarg, NULL, 0) < 0) {
					usage();
					rc = -1;
					goto exit;
				}
				rc = srp_latency_parameter_table(ctrl_h, port, tc_key);
			}
			break;

		case 'h':
		default:
			usage();
			rc = -1;
			goto exit;
		}
	}

exit:
	if (endpoint_msrp_ctrl_h)
		genavb_control_close(endpoint_msrp_ctrl_h);

	if (bridge_msrp_ctrl_h)
		genavb_control_close(bridge_msrp_ctrl_h);
err_control_open:
	return rc;
}

