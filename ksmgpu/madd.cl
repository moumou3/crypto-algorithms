__kernel void madd(global int *matrix1, global int *matrix2, global int *sum, int packet_num)
{
	int i = get_global_id(0);

        if (i < packet_num)
          sum[i] = 2;
}
