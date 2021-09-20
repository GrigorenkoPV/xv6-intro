#include "kernel/types.h"
#include "user/user.h"

#define PINGPONG_SUCCESS 0
#define PINGPONG_PIPE_FAILURE 1
#define PINGPONG_FORK_FAILURE 2
#define PINGPONG_IO_FAILURE 3
#define PINGPONG_MEMORY_ALLOCATION_FAILURE 4

//#define DEBUG

#ifdef DEBUG
#define LOG(...)                  \
  do {                            \
    sleep(getpid() == 3 ? 3 : 5); \
    printf("PID %d: ", getpid()); \
    printf(__VA_ARGS__);          \
    printf("\n");                 \
  } while (0);
#else
#define LOG(...) (void)(0);
#endif

int realloc_buffer(char **buffer, uint *buffer_size, uint len) {
  uint new_buffer_size = *buffer_size ? 2 * *buffer_size : 1;
  char *new_buffer = malloc(new_buffer_size);
  if (!new_buffer) {
    return PINGPONG_MEMORY_ALLOCATION_FAILURE;
  }
  memcpy(new_buffer, *buffer, len);
  free(*buffer);
  *buffer = new_buffer;
  *buffer_size = new_buffer_size;
  return PINGPONG_SUCCESS;
}

int read_all(int fd, char **result) {
  uint buffer_size = 4;
  uint total_read = 0;
  char *buffer = malloc(buffer_size);
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
    LOG("Read %d characters", characters_read);
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

int receive_message(int fd) {
  LOG("Receiving message from file %d and printing it out", fd);
  char *msg = 0;
  int rc = read_all(fd, &msg);
  if (rc != PINGPONG_SUCCESS) {
    return rc;
  }
  LOG("Message received!")
  printf("%d: got %s\n", getpid(), msg);
  free(msg);
  return PINGPONG_SUCCESS;
}

int parent(int read_fd, int write_fd) {
  LOG("Entered parent: read_fd = %d, write_fd = %d.", read_fd, write_fd);
  if (write(write_fd, &"ping\0", 4) != 4) {
    return PINGPONG_IO_FAILURE;
  }
  LOG("Wrote message into file %d", write_fd);
  if (close(write_fd)) {
    return PINGPONG_IO_FAILURE;
  }
  LOG("Closed file %d", write_fd);
  return receive_message(read_fd);
}

int child(int read_fd, int write_fd) {
  LOG("Entered child: read_fd = %d, write_fd = %d.", read_fd, write_fd);
  int rc = receive_message(read_fd);
  if (rc != PINGPONG_SUCCESS) {
    return rc;
  }
  LOG("Received message from file %d", read_fd);
  if (close(read_fd)) {
    return PINGPONG_IO_FAILURE;
  }
  LOG("Closed file %d", read_fd);
  if (write(write_fd, &"pong", 4) != 4) {
    return PINGPONG_IO_FAILURE;
  }
  LOG("Wrote message into file %d", write_fd);
  if (close(write_fd)) {
    return PINGPONG_IO_FAILURE;
  }
  return PINGPONG_SUCCESS;
}

#define READING_END 0
#define WRITING_END 1

int pingpong() {
  int parent_to_child_pipefd[2];
  int child_to_parent_pipefd[2];

  if (pipe(parent_to_child_pipefd)) {
    return PINGPONG_PIPE_FAILURE;
  }
  LOG("Initialized parent_to_child_pipe: %d, %d.", parent_to_child_pipefd[0],
      parent_to_child_pipefd[1]);
  if (pipe(child_to_parent_pipefd)) {
    return PINGPONG_PIPE_FAILURE;
  }
  LOG("Initialized child_to_parent_pipe: %d, %d.", child_to_parent_pipefd[0],
      child_to_parent_pipefd[1]);

  switch (fork()) {
    case -1:
      return PINGPONG_FORK_FAILURE;
    case 0:  // child
      close(parent_to_child_pipefd[WRITING_END]);
      close(child_to_parent_pipefd[READING_END]);
      return child(parent_to_child_pipefd[READING_END],
                   child_to_parent_pipefd[WRITING_END]);
    default:  // parent
      close(child_to_parent_pipefd[WRITING_END]);
      close(parent_to_child_pipefd[READING_END]);
      return parent(child_to_parent_pipefd[READING_END],
                    parent_to_child_pipefd[WRITING_END]);
  }
}

int main(int argc, char *argv[]) {
  int rc = pingpong();
  switch (rc) {
    case PINGPONG_PIPE_FAILURE:
      fprintf(2, "Failed to create pipes.\n");
      break;
    case PINGPONG_FORK_FAILURE:
      fprintf(2, "Failed to fork process.\n");
      break;
    case PINGPONG_IO_FAILURE:
      fprintf(2, "I/O error.\n");
      break;
    case PINGPONG_MEMORY_ALLOCATION_FAILURE:
      fprintf(2, "Failed to allocate memory.\n");
      break;
  }
  exit(rc);
}
