#ifndef CAFFE_SYNCEDMEM_HPP_
#define CAFFE_SYNCEDMEM_HPP_

#include <cstdlib>
#include <assert.h>
#ifdef USE_MKL
#include "mkl.h"
#endif

#include "caffe/common.hpp"

namespace caffe {
    
    // If CUDA is available and in GPU mode, host memory will be allocated pinned,
    // using cudaMallocHost. It avoids dynamic pinning for transfers (DMA).
    // The improvement in performance seems negligible in the single GPU case,
    // but might be more significant for parallel training. Most importantly,
    // it improved stability for large models on many GPUs.
    inline void CaffeMallocHost(void** ptr, size_t size, bool* use_cuda, size_t aligned_size) {
#ifndef CPU_ONLY
        if (Caffe::mode() == Caffe::GPU) {
            CUDA_CHECK(cudaMallocHost(ptr, aligned_size));
            *use_cuda = true;
            return;
        }
#endif
#ifdef USE_MKL
        *ptr = mkl_malloc(size ? size:1, 64);
#else
        posix_memalign(ptr, 4096, aligned_size);
#endif
        *use_cuda = false;
        CHECK(*ptr) << "host allocation of size " << aligned_size << " failed";
    }
    
    inline void CaffeFreeHost(void* ptr, bool use_cuda) {
#ifndef CPU_ONLY
        if (use_cuda) {
            CUDA_CHECK(cudaFreeHost(ptr));
            return;
        }
#endif
#ifdef USE_MKL
        mkl_free(ptr);
#else
        free(ptr);
#endif
    }
    
    
    /**
     * @brief Manages memory allocation and synchronization between the host (CPU)
     *        and device (GPU).
     *
     * TODO(dox): more thorough description.
     */
    class SyncedMemory {
    public:
        SyncedMemory();
        explicit SyncedMemory(size_t size);
        ~SyncedMemory();
        const void* cpu_data();
        void set_cpu_data(void* data);
        const void* gpu_data();
        void set_gpu_data(void* data);
        void* mutable_cpu_data();
        void* mutable_gpu_data();
        enum SyncedHead { UNINITIALIZED, HEAD_AT_CPU, HEAD_AT_GPU, SYNCED };
        SyncedHead head() { return head_; }
        size_t size() { return size_; }
        
        void default_reference();
        void increase_reference();
        void decrease_reference();
        void zhihan_release();
        
#ifndef CPU_ONLY
        void async_gpu_push(const cudaStream_t& stream);
#endif
        
    private:
        void check_device();
        int refer_num;
        void to_cpu();
        void to_gpu();
        void* cpu_ptr_;
        void* gpu_ptr_;
        size_t size_;
        size_t aligned_size_;
        SyncedHead head_;
        bool own_cpu_data_;
        bool cpu_malloc_use_cuda_;
        bool own_gpu_data_;
        int device_;
        inline void set_align_size(size_t size){
            aligned_size_ = ((size + 4095) / 4096) * 4096;
        };
        inline size_t get_align_size(){
            return aligned_size_;
        };
        DISABLE_COPY_AND_ASSIGN(SyncedMemory);
    };  // class SyncedMemory
    
}  // namespace caffe

#endif  // CAFFE_SYNCEDMEM_HPP_