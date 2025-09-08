#ifndef PTY_H
#define PTY_H 1

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

typedef struct pty_config {
  int master_fd;
  int slave_fd;
  char slave_name[256];
  int is_connected;
} pty_config;

int pty_get_bps_const(int speed);
int pty_init_conn(char *name_hint, int speed, pty_config *pty_cfg);
int pty_set_flow_control(int fd, int status);
int pty_get_control_lines(int fd);
int pty_set_control_lines(int fd, int state);
int pty_write(int fd, unsigned char *data, int len);
int pty_read(int fd, unsigned char *data, int len);
void pty_cleanup(pty_config *pty_cfg);

#endif