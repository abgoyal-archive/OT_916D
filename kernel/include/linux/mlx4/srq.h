

#ifndef MLX4_SRQ_H
#define MLX4_SRQ_H

struct mlx4_wqe_srq_next_seg {
	u16			reserved1;
	__be16			next_wqe_index;
	u32			reserved2[3];
};

#endif /* MLX4_SRQ_H */
