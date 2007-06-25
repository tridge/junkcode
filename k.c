main()
{

	fd_set fds;

	
	while (!read_all_data) {
		tval.tv_sec = 1;
		tval.tv_usec = 0;
		FD_ZERO(&fds);
		FS_SET(&fds, s);
		ret = select(s+1, &fds, NULL, NULL, &tval);
		if (ret > 0) {
			if (FD_ISSET(&fds, s)) {
				read(s, lksnfglkm);
			}
		}
		do_background_processing();
	}

}
