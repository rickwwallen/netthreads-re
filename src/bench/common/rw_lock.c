#include "support.h"

void rw_lock_init(rw_lock* rw, int lock)
{
    rw->lock = lock;
    rw->writing = 0;
    rw->writer_pending = 0;
    rw->writer_id = -1;
    rw->reader_cnt = 0;
}


void reader_lock(rw_lock* rw)
{
    int success = 0;   // local var
    while(!success)
    {
        while(rw->writing || rw->writer_pending)
	  relax();

        nf_lock(rw->lock);
            if(!(rw->writing || rw->writer_pending))
            {
                   rw->reader_cnt++;
                   success = 1;
            }
        nf_unlock(rw->lock);
    }
}

void reader_unlock(rw_lock* rw)
{
    nf_lock(rw->lock);
        rw->reader_cnt--;
    nf_unlock(rw->lock);
}


/* ------ WRITER ------ */
void writer_lock(rw_lock* rw)
{
    int success = 0;   // local var
    int mytid = nf_tid();
   
    while(!success)
    {
        while(rw->writer_pending && (mytid != rw->writer_id))
	  relax();
        while(rw->reader_cnt || rw->writing)
	  relax();
       
        nf_lock(rw->lock);
            if(!(rw->writer_pending)) {
                rw->writer_pending = 1;
                rw->writer_id = mytid;
            }
            if(!(rw->reader_cnt || rw->writing))
            {
                   rw->writing = 1;
                   success = 1;
            }
        nf_unlock(rw->lock);
    }
}

void writer_unlock(rw_lock* rw)
{
    nf_lock(rw->lock);
        rw->writer_pending = 0;
        rw->writer_id = -1;
        rw->writing = 0;
    nf_unlock(rw->lock);
}

