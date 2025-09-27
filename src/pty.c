#define _GNU_SOURCE
#include <sys/file.h>
#include <unistd.h>
#include <termios.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <pty.h>
#include <string.h>
#include "dce.h"
#include "debug.h"
#include "serial.h"

#include "pty.h"

int pty_get_bps_const(int speed) {
  // Reuse the existing serial speed constants
  return ser_get_bps_const(speed);
}

int pty_init_conn(char *name_hint, int speed, pty_config *pty_cfg) {
  int master_fd = -1;
  int slave_fd = -1;
  char *slave_name = NULL;
  struct termios tio;
  int bps_rate = 0;

  LOG_ENTER();

  bps_rate = pty_get_bps_const(speed);

  if(bps_rate > -1) {
    /* Check if name_hint specifies a specific PTY device */
    if (name_hint && strncmp(name_hint, "/dev/pts/", 9) == 0) {
      /* User specified a specific PTY device */
      LOG(LOG_INFO, "Opening specified PTY device %s at speed %d", name_hint, speed);
      
      master_fd = open(name_hint, O_RDWR | O_NOCTTY | O_NONBLOCK);
      if (master_fd < 0) {
        ELOG(LOG_FATAL, "Could not open specified PTY device %s", name_hint);
        LOG_EXIT();
        return -1;
      }
      
      slave_fd = master_fd;  /* For specified PTY, master and slave are the same */
      strncpy(pty_cfg->slave_name, name_hint, sizeof(pty_cfg->slave_name) - 1);
      pty_cfg->slave_name[sizeof(pty_cfg->slave_name) - 1] = '\0';
      
      LOG(LOG_INFO, "Opened specified PTY device: %s as fd %d", name_hint, master_fd);
    } else {
      /* Create a pseudo terminal pair automatically */
      LOG(LOG_INFO, "Creating pseudo terminal with speed %d", speed);

      if (openpty(&master_fd, &slave_fd, NULL, NULL, NULL) < 0) {
        ELOG(LOG_FATAL, "Could not create pseudo terminal pair"); 
        LOG_EXIT();
        return -1;
      }

      slave_name = ttyname(slave_fd);
      if (slave_name == NULL) {
        ELOG(LOG_FATAL, "Could not get slave PTY name");
        close(master_fd);
        close(slave_fd);
        LOG_EXIT();
        return -1;
      }

      strncpy(pty_cfg->slave_name, slave_name, sizeof(pty_cfg->slave_name) - 1);
      pty_cfg->slave_name[sizeof(pty_cfg->slave_name) - 1] = '\0';
      
      LOG(LOG_INFO, "Created pseudo terminal: master_fd=%d, slave=%s", master_fd, slave_name);
    }

    pty_cfg->master_fd = master_fd;
    pty_cfg->slave_fd = slave_fd;
    pty_cfg->is_connected = TRUE;

    /* Make the file descriptor non-blocking */
    fcntl(master_fd, F_SETFL, O_NONBLOCK | FASYNC);

    /* Configure the terminal settings if we created a new PTY */
    if (slave_name != NULL) {
      if (0 != tcgetattr(slave_fd, &tio)) {
        ELOG(LOG_WARN, "Could not get PTY slave attributes, using defaults");
      } else {
        // Configure similar to serial port but without hardware flow control
        tio.c_cflag = CS8 | CLOCAL | CREAD;
        tio.c_iflag = IGNBRK;
        tio.c_oflag = 0;
        tio.c_lflag = 0;
        cfsetispeed(&tio, bps_rate);
        cfsetospeed(&tio, bps_rate);

        tio.c_cc[VMIN] = 1;
        tio.c_cc[VTIME] = 0;

        tcflush(slave_fd, TCIFLUSH);
        if (tcsetattr(slave_fd, TCSANOW, &tio) != 0) {
          ELOG(LOG_WARN, "Could not set PTY slave attributes");
        } else {
          LOG(LOG_INFO, "PTY slave configured successfully");
        }
      }
    } else {
      /* For specified PTY devices, try to configure them but don't fail if it doesn't work */
      if (0 == tcgetattr(master_fd, &tio)) {
        cfsetispeed(&tio, bps_rate);
        cfsetospeed(&tio, bps_rate);
        tcsetattr(master_fd, TCSANOW, &tio);
        LOG(LOG_INFO, "Specified PTY device configured successfully");
      }
    }

    // Print the PTY device name for user reference
    printf("Pseudo terminal device: %s\n", pty_cfg->slave_name);
    fflush(stdout);
  }

  LOG_EXIT();
  return master_fd;
}

int pty_set_flow_control(int fd, int status) {
  struct termios tio;
  if(0 != tcgetattr(fd, &tio)) {
    ELOG(LOG_WARN, "Could not get PTY attributes for flow control");
    return -1;
  }
  // turn all off first
  tio.c_cflag &= ~(IXON | IXOFF | CRTSCTS);
  tio.c_cflag |= status;
  if(0 != tcsetattr(fd, TCSANOW, &tio)) {
    ELOG(LOG_WARN,"Could not set PTY attributes for flow control");
    return -1;
  }
  return 0;
}

int pty_get_control_lines(int fd) {
  // For PTY, we simulate that the link is always up and DTR is active
  // without trying to read the actual TIOCM status since it's not meaningful for PTY
  return (DCE_CL_LE | DCE_CL_DTR);
}

int pty_set_control_lines(int fd, int state) {
  // For PTY, control line simulation is limited, just return success
  // without trying to set the actual TIOCM status
  return 0;
}

int pty_write(int fd, unsigned char* data, int len) {
  log_trace(TRACE_MODEM_OUT, data, len);
  return write(fd, data, len);
}

int pty_read(int fd, unsigned char* data, int len) {
  int res;

  res = read(fd, data, len);
  log_trace(TRACE_MODEM_IN, data, res);
  return res;
}

void pty_cleanup(pty_config *pty_cfg) {
  if (pty_cfg->is_connected) {
    if (pty_cfg->master_fd >= 0) {
      close(pty_cfg->master_fd);
      pty_cfg->master_fd = -1;
    }
    /* Only close slave_fd if it's different from master_fd (auto-created PTY) */
    if (pty_cfg->slave_fd >= 0 && pty_cfg->slave_fd != pty_cfg->master_fd) {
      close(pty_cfg->slave_fd);
      pty_cfg->slave_fd = -1;
    }
    pty_cfg->is_connected = FALSE;
    LOG(LOG_INFO, "PTY cleaned up");
  }
}