#ifndef OVERLAY_H
#define OVERLAY_H

// merged_dir buffer must be at least 256 bytes
int  setup_overlay(const char *base_image, const char *container_id, char *merged_dir);
void cleanup_overlay(const char *container_id);

#endif
