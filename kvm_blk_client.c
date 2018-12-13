#include "kvm_blk.h"
extern void kvm_blk_read_ready(void *opaque);
extern uint32_t debug_flag;
extern struct kvm_blk_request *wreq_curr;
extern pthread_cond_t pending_cond;
extern pthread_mutex_t pending_mutex; 
bool wreq_is_pending = false;

int kvm_blk_client_handle_cmd(void *opaque)
{
    KvmBlkSession *s = opaque;
	struct kvm_blk_request *br;
	uint32_t cmd = s->recv_hdr.cmd;
	int32_t id = s->recv_hdr.id;
	int ret = 0;


	if (debug_flag == 1) {
    	debug_printf("received cmd %d len %d id %d\n", cmd, s->recv_hdr.payload_len,
					s->recv_hdr.id);
    }

	if (cmd == KVM_BLK_CMD_COMMIT_ACK) {
		if (s->ack_cb)
			s->ack_cb(s->ack_cb_opaque);
		return 0;
	}

	QTAILQ_FOREACH(br, &s->request_list, node)
		if (br->id == id)
			break;

	if (!br) {
		printf("%s can't find record for cmd = %d id = %d\n",
				__func__, s->recv_hdr.cmd,id);
		return -1;
	}

    qemu_mutex_lock(&s->mutex);
	QTAILQ_REMOVE(&s->request_list, br, node);
    qemu_mutex_unlock(&s->mutex);
	// handle WRITE
	if (s->recv_hdr.cmd == KVM_BLK_CMD_WRITE) {
				br->cb(br->opaque, 0);
				if(debug_flag == 2) {
					printf("[receive]write %ld %d %d\n",(long)br->sector,br->nb_sectors,s->recv_hdr.id);
				}
        // for quick write
        goto out;
	}

	// handle SYNC_READ
	if (br->cb == NULL) {
		// hack for kvm_blk_rw_co
		br->cb = (void *)0xFFFFFFFF;
		goto out;
	}

	// handle READ
	if (s->recv_hdr.payload_len < 0) {
		br->cb(br->opaque, s->recv_hdr.payload_len);
		goto out;
	}
	
	if (s->recv_hdr.payload_len != br->nb_sectors) {
		fprintf(stderr, "%s expect %d, get %d\n", __func__,
				br->nb_sectors, s->recv_hdr.payload_len);
	}

	kvm_blk_input_to_iov(s, br->piov);
	br->cb(br->opaque, 0);
	if(debug_flag == 2) {
			printf("[receive]read %ld %d %d\n",(long)br->sector,br->nb_sectors,s->recv_hdr.id);
	}
out:

		if(s->recv_hdr.cmd == KVM_BLK_CMD_WRITE) {
				//record this wreq
				if(wreq_curr != NULL)
					free(wreq_curr);
				wreq_curr = g_malloc(sizeof(*br));
				memcpy(wreq_curr,br,sizeof(*br));
				//does ready to take snapshot?
				/*	
				if(wreq_is_pending) {
					QTAILQ_FOREACH(brtmp, &s->request_list, node) 
							if (brtmp->cmd == KVM_BLK_CMD_WRITE) 
									goto free;
					pthread_cond_signal(&pending_cond);
				}
				*/
		}						        
	if(br)
		g_free(br);
  	return ret;
}

struct kvm_blk_request *kvm_blk_aio_readv(BlockBackend *blk,
                                        int64_t sector_num,
                                        QEMUIOVector *iov,
                                        BdrvRequestFlags flags,
                                        BlockCompletionFunc *cb,
                                        void *opaque)
{
	KvmBlkSession *s = kvm_blk_session;
	struct kvm_blk_read_control c;
	struct kvm_blk_request *br;

	assert(s->bs = blk_bs(blk));
	br = g_malloc0(sizeof(*br));
	br->sector = sector_num;
	br->nb_sectors = iov->size;
	br->cmd = KVM_BLK_CMD_READ;
	br->session = s;
	br->flags = flags;
	br->piov = iov;
	br->cb = cb;
	br->opaque = opaque;

	c.sector_num = sector_num;
	c.nb_sectors = iov->size;

    qemu_mutex_lock(&s->mutex);

	br->id = ++s->id;
	QTAILQ_INSERT_TAIL(&s->request_list, br, node);
	s->send_hdr.cmd = KVM_BLK_CMD_READ;
	s->send_hdr.payload_len = sizeof(c);
	s->send_hdr.id = s->id;
	s->send_hdr.num_reqs = 1;

	s->send_hdr.flags = flags;
	s->send_hdr.cb = cb;
	s->send_hdr.opaque = opaque;
	s->send_hdr.piov = iov; 

	kvm_blk_output_append(s, &s->send_hdr, sizeof(s->send_hdr));
  	kvm_blk_output_append(s, &c, sizeof(c));
  	kvm_blk_output_flush(s);
    qemu_mutex_unlock(&s->mutex);

    if (debug_flag == 1) {
		debug_printf("send read cmd: %ld %d %d\n", (long)c.sector_num, c.nb_sectors, s->id);
	}

		if(debug_flag == 2) {
				printf("[send]read sector:%ld size:%d id:%d opaque:%p\n", (long)c.sector_num, c.nb_sectors, s->id,opaque);
		}
	return br;
}

struct kvm_blk_request *kvm_blk_aio_write(BlockBackend *blk,int64_t sector_num,QEMUIOVector *iov, BdrvRequestFlags flags,BlockCompletionFunc *cb,void *opaque){
	KvmBlkSession *s = kvm_blk_session;
	struct kvm_blk_read_control c;
	struct kvm_blk_request *br;
	assert(s->bs = blk_bs(blk));
	br = g_malloc0(sizeof(*br));
	br->sector = sector_num;
	br->nb_sectors = iov->size;
	br->cmd = KVM_BLK_CMD_WRITE;
	br->session = s;
	br->flags = flags;
	br->piov = iov;
	br->cb = cb;
	br->opaque = opaque;

    qemu_mutex_lock(&s->mutex);
	br->id = ++s->id;
	write_request_id = s->id;
	QTAILQ_INSERT_TAIL(&s->request_list, br, node);

	s->send_hdr.cmd = KVM_BLK_CMD_WRITE;
	s->send_hdr.payload_len = sizeof(c)+iov->size;
	s->send_hdr.id = s->id;
	s->send_hdr.num_reqs = 1;
	
	s->send_hdr.flags = flags;
	s->send_hdr.cb = cb;
	s->send_hdr.opaque = opaque; 
	s->send_hdr.piov = iov; 

	kvm_blk_output_append(s, &s->send_hdr, sizeof(s->send_hdr));
	
	c.sector_num = sector_num;
	c.nb_sectors = iov->size;

	kvm_blk_output_append(s, &c, sizeof(c));
	kvm_blk_output_append_iov(s, iov);
	kvm_blk_output_flush(s);

    qemu_mutex_unlock(&s->mutex);
	//cb(opaque, 0);
	if(debug_flag == 2) {
			printf("[send]write sector:%ld size:%d id:%d opaque:%p\n", (long)c.sector_num, c.nb_sectors, s->id,opaque);
	}
	return br;
}


static void _kvm_blk_send_cmd(KvmBlkSession *s, int cmd)
{
    qemu_mutex_lock(&s->mutex);

    s->send_hdr.cmd = cmd;
		if(cmd == KVM_BLK_CMD_EPOCH_TIMER)	
				s->send_hdr.id = wreq_curr->id;
	s->send_hdr.payload_len = 0;

	kvm_blk_output_append(s, &s->send_hdr, sizeof(s->send_hdr));
	kvm_blk_output_flush(s);

    qemu_mutex_unlock(&s->mutex);
}

void kvm_blk_epoch_timer(KvmBlkSession *s)
{
		
		if(debug_flag == 2) {
				printf("epoch:%d \n",wreq_curr->id);
		}

    _kvm_blk_send_cmd(s, KVM_BLK_CMD_EPOCH_TIMER);
}

void kvm_blk_epoch_commit(KvmBlkSession *s)
{
		if(debug_flag == 2) {
	    printf("commit\n");
		}
    _kvm_blk_send_cmd(s, KVM_BLK_CMD_COMMIT);
}

void kvm_blk_notify_ft(KvmBlkSession *s)
{
    _kvm_blk_send_cmd(s, KVM_BLK_CMD_FT);
}


void kvm_blk_wait_pending_wreq(KvmBlkSession *s) {
	//struct kvm_blk_request *br;
	/*
	QTAILQ_FOREACH(br, &s->request_list, node) {
		if (br->cmd == KVM_BLK_CMD_WRITE) {
			wreq_is_pending = true;
			return;
		}
	}
	wreq_is_pending = false;
	 */
	//v1
	/*
	while(1) {
		QTAILQ_FOREACH(br, &s->request_list, node)
				if (br->cmd == KVM_BLK_CMD_WRITE)
						break;
		if(br == NULL)
			break;
		qemu_mutex_lock(&s->mutex);
		QTAILQ_REMOVE(&s->request_list, br, node);
		qemu_mutex_unlock(&s->mutex);
    br->cb(br->opaque, 0);
		memcpy(wreq_curr,br,sizeof(*br));
		free(br);
	}
	*/
	return;
}

