#ifndef __ANDROID__
#include "DiceGit.h"
#include "DiceFile.hpp"
#include "DiceConsole.h"
DiceRepo::DiceRepo(const std::filesystem::path& dir) {
	if (!std::filesystem::exists(dir)) {
		init(getNativePathString(dir));
	}
	else if (!std::filesystem::exists(dir / ".git")) {
		init(getNativePathString(dir));
	}
	else {
		open(getNativePathString(dir));
	}
}
DiceRepo::DiceRepo(const std::filesystem::path& dir, const string& url) {
	clone(getNativePathString(dir), url);
}
DiceRepo::~DiceRepo() {
	if (repo)git_repository_free(repo);
	if (remote)git_remote_free(remote);
}
DiceRepo& DiceRepo::init(const string& dir) {
	git_repository_init(&repo, dir.c_str(), 0);
	return *this;
}
int cred_acquire_cb(git_cred** cred, const char* url, const char* username_from_url, unsigned int allowed_types, void* payload) {
	return git_credential_userpass_plaintext_new(cred,
		console.git_user.c_str(), console.git_pw.c_str());
}
string git_lasterr() {
	return UTF8toGBK(git_error_last()->message);
}
DiceRepo& DiceRepo::open(const string& dir) {
	git_repository_open(&repo, dir.c_str()); 
	git_remote_callbacks remote_conn_opt = GIT_REMOTE_CALLBACKS_INIT;
	remote_conn_opt.credentials = cred_acquire_cb;
	git_remote_connect(remote, GIT_DIRECTION_FETCH, &remote_conn_opt, nullptr, nullptr);
	return *this;
}
DiceRepo& DiceRepo::clone(const string& local_path, const string& url) {
	git_clone_options options = GIT_CLONE_OPTIONS_INIT;
	options.fetch_opts.callbacks.credentials = cred_acquire_cb;
	if (git_clone(&repo, url.c_str(), local_path.c_str(), &options))
		console.log("git_clone_err " + to_string(git_error_last()->klass) + ":" + UTF8toGBK(git_error_last()->message), 1);
	return *this;
}
DiceRepo& DiceRepo::url(const string& link) {
	//remote_url = link;
	//git_fetch_options fetch_opts = GIT_FETCH_OPTIONS_INIT;
	git_remote_create(&remote, repo, "origin", link.c_str());
	return *this;
}
bool DiceRepo::update(string& err) {
	git_reference* local_head = nullptr;  //refs/heads/master 'HEAD'
	try {
		git_repository_head(&local_head, repo);
		const char* branch_name{ nullptr };
		if (git_branch_name(&branch_name, local_head)) {
			console.log("git_branch_name:" + (err = git_lasterr()), 1);
			goto Clean;
		}
		//string head{ git_reference_name(local_head) };
		//git_checkout_head(repo,NULL);
		//fetch
		git_fetch_options fetch_opts = GIT_FETCH_OPTIONS_INIT;
		git_remote_lookup(&remote, repo, "origin");
		fetch_opts.callbacks.credentials = cred_acquire_cb;
		fetch_opts.prune = GIT_FETCH_PRUNE;
		if (git_remote_fetch(remote, nullptr, &fetch_opts, nullptr)) {
			console.log("git_remote_fetch:" + (err = git_lasterr()), 1);
			goto Clean;
		}
		//set head
		git_reference* origin_head = nullptr; //refs/remotes/origin/master
		string remote_branch{ "origin/" + string(branch_name) };
		git_branch_lookup(&origin_head, repo,
			remote_branch.c_str(), GIT_BRANCH_REMOTE);
		//string origin_ref{ git_reference_name(origin_head) };
		const git_oid* local_id{ git_reference_target(local_head) };
		const git_oid* origin_id{ git_reference_target(origin_head) };
		if (!strcmp((char*)local_id, (char*)origin_id)) {
			err = "up to date already";
			goto Clean;
		}
		//merge
		git_merge_options merge_opt = GIT_MERGE_OPTIONS_INIT;
		git_checkout_options checkout_opt = GIT_CHECKOUT_OPTIONS_INIT;
		git_annotated_commit* their_head[10]{ nullptr };
		git_annotated_commit_from_ref(their_head, repo, origin_head);
		if (git_merge(repo, (const git_annotated_commit**)their_head, 1, &merge_opt, &checkout_opt)) {
			console.log("git_merge_err:" + (err = git_lasterr()), 1);
		}
		git_annotated_commit_free(their_head[0]);
		//set_target
		git_reference* new_target_ref{ nullptr };
		/* Grab the reference HEAD should be pointing to */
		git_reference* target_ref{ nullptr };
		if (git_reference_set_target(&new_target_ref, local_head, origin_id, nullptr)) {
			console.log("git_set_target_err:" + (err = git_lasterr()), 1);
		}
		git_reference_free(new_target_ref);
		//index
		/*git_index* index = nullptr;
		git_repository_index(&index, repo);
		git_index_update_all(index, nullptr, nullptr, nullptr);
		git_index_write(index);
		git_oid new_tree_id;
		git_tree* new_tree = nullptr;
		git_index_write_tree(&new_tree_id, index);
		git_tree_lookup(&new_tree, repo, &new_tree_id);
		git_index_free(index);*/
		if (git_checkout_head(repo, &checkout_opt)) {
			console.log("git_checkout_err:" + (err = git_lasterr()), 1);
		}
		//rebase
		/*git_rebase* prebase = nullptr;
		git_rebase_options rebase_opt = GIT_REBASE_OPTIONS_INIT;
		git_annotated_commit* onto = nullptr;
		git_annotated_commit_from_ref(&onto, repo, local_head);
		git_rebase_init(&prebase, repo, nullptr, nullptr
			, onto, &rebase_opt);
		git_rebase_operation* operation = nullptr;
		while (git_rebase_next(&operation, prebase) != GIT_ITEROVER);*/
	} catch (std::exception& e) {
		console.log("exception:" + (err = e.what()), 1);
	}
Clean:	//clean
	git_reference_free(local_head);
	git_repository_state_cleanup(repo);
	return err.empty();
}
#endif //__ANDROID__