typedef struct {
    union
    {
        unsigned int addr;
        struct{
            unsigned short net;
            unsigned char  node;
            unsigned char  pad; // for alignment
        };
    };
    char   zone[48];
    char  *intf_name; // for what netcard? 0 = no netcard, else pointer to "eth0", or "eth1", or "bond0".
} atalkd_config_ini;


main()
{
	atalkd_config_ini a;

	a.net = 3;
}
