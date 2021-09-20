#include "kernel/types.h"
#include "user/user.h"

#define PINGPONG_SUCCESS 0
#define PINGPONG_PIPE_FAILURE 1
#define PINGPONG_FORK_FAILURE 2
#define PINGPONG_IO_FAILURE 3
#define PINGPONG_MEMORY_ALLOCATION_FAILURE 4

int realloc_buffer(char** buffer, uint* buffer_size, uint len) {
  uint new_buffer_size = *buffer_size ? 2 * *buffer_size : 1;
  char* new_buffer = malloc(new_buffer_size);
  if (!new_buffer) {
    return PINGPONG_MEMORY_ALLOCATION_FAILURE;
  }
  memcpy(new_buffer, *buffer, len);
  free(*buffer);
  *buffer = new_buffer;
  *buffer_size = new_buffer_size;
  return PINGPONG_SUCCESS;
}

int read_all(int fd, char** result) {
  uint buffer_size = 4;
  uint total_read = 0;
  char* buffer = malloc(buffer_size);
  if (!buffer) {
    return PINGPONG_MEMORY_ALLOCATION_FAILURE;
  }
  int characters_read = 0;
  do {
    total_read += characters_read;
    if (buffer_size == total_read) {
      int rc = realloc_buffer(&buffer, &buffer_size, total_read);
      if (rc != PINGPONG_SUCCESS) {
        return rc;
      }
    }
    characters_read = read(fd, buffer + total_read, buffer_size - total_read);
  } while (characters_read > 0);
  if (characters_read) {
    free(buffer);
    return PINGPONG_IO_FAILURE;
  }
  if (total_read == 0 || buffer[total_read - 1 != 0]) {
    if (buffer_size == total_read) {
      int rc = realloc_buffer(&buffer, &buffer_size, total_read);
      if (rc != PINGPONG_SUCCESS) {
        return rc;
      }
    }
    buffer[total_read++] = 0;
  }
  *result = buffer;
  return PINGPONG_SUCCESS;
}

int print_message(int fd) {
  char* msg = 0;
  int rc = read_all(fd, &msg);
  if (rc != PINGPONG_SUCCESS) {
    return rc;
  }
  printf("%d: got %s", getpid(), msg);
  free(msg);
  return PINGPONG_SUCCESS;
}

int parent(int read_fd, int write_fd) {
  if (write(write_fd, &"ping", 4) != 4 || close(write_fd)) {
    return PINGPONG_IO_FAILURE;
  }
  return print_message(read_fd);
}

int child(int read_fd, int write_fd) {
  int rc = print_message(read_fd);
  if (rc != PINGPONG_SUCCESS) {
    return rc;
  }
  if (write(write_fd, &"pong", 4) != 4 || close(write_fd)) {
    return PINGPONG_IO_FAILURE;
  }
  return PINGPONG_SUCCESS;
}

int pingpong() {
  int parent_to_child_pipefd[2];
  int child_to_parent_pipefd[2];

  if (pipe(parent_to_child_pipefd)) {
    return PINGPONG_PIPE_FAILURE;
  }
  if (pipe(child_to_parent_pipefd)) {
    return PINGPONG_PIPE_FAILURE;
  }

  switch (fork()) {
    case -1:
      return PINGPONG_FORK_FAILURE;
    case 0:
      return child(parent_to_child_pipefd[0], child_to_parent_pipefd[1]);
    default:
      return parent(child_to_parent_pipefd[0], parent_to_child_pipefd[1]);
  }
}

int main(int argc, char* argv[]) { exit(pingpong()); }
