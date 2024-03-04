#define CBS_IMPLEMENTATION
#include "cbs.h"

#define CC "cc"
#define CFLAGS "-Wall", "-Wextra", "-Wpedantic", "-I./src", "-c"
#define TARGET_NAME "hush"

#define FOR_OPTIONS(DO) \
	DO(build) \
	DO(run) \
	DO(clean) \

void build(void) {
	
	// Compile source files
	Cbs_File_Paths src_paths = {0}, obj_paths = {0};
	cbs_file_paths_build_file_ext(&src_paths, "./src", ".c");
	if (!cbs_file_exists("./obj")) cbs_run("mkdir", "./obj");
	cbs_file_paths_for_each(src_path, src_paths) {
		const char *obj_name = cbs_strip_file_ext(cbs_get_file_name(src_path));
		const char *obj_path = cbs_string_build("./obj/", obj_name, ".o");
		cbs_file_paths_append(&obj_paths, obj_path);
		if (cbs_needs_rebuild(obj_path, src_path))
			cbs_run(CC, CFLAGS, "-o", obj_path, src_path);
	}
	cbs_file_paths_free(&src_paths);

	// Link object files
	if (!cbs_file_exists("./bin")) cbs_run("mkdir", "./bin");
	const char *target_path = cbs_string_build("./bin/", TARGET_NAME);
	if (cbs_needs_rebuild_file_paths(target_path, obj_paths)) {
		Cbs_Cmd cmd = {0};
		cbs_cmd_build(&cmd, CC, "-o", target_path);
		cbs_cmd_build_file_paths(&cmd, obj_paths);
		cbs_cmd_run(&cmd);
	}
	cbs_file_paths_free(&obj_paths);
}

void run(void) {
	Cbs_File_Paths src_paths = {0};
	cbs_file_paths_build_file_ext(&src_paths, "./src", ".c");
	const char *target_path = cbs_string_build("./bin/", TARGET_NAME);
	if (cbs_needs_rebuild_file_paths(target_path, src_paths)) build();
	cbs_run(target_path);
}

void clean(void) {
	cbs_run("rm", "-rf", "./bin", "./obj");
}

int main(int argc, char **argv) {
	cbs_rebuild_self(argv);
	cbs_shift_args(&argc, &argv);

	const char *opt;
	while ((opt = cbs_shift_args(&argc, &argv))) {
#define CHECK_OPTION(name) if (cbs_string_eq(opt, #name)) { name(); continue; }
		FOR_OPTIONS(CHECK_OPTION)
	}
	
	return 0;
}
