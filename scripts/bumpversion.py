# coding=utf-8
# This work is licensed!
# pylint: disable=W0702,R0201,C0111,W0613,

"""
  Main entry point.
"""
import getopt
import os
import sys
import re
import time
import codecs
import logging
import io
PY2 = (2 == sys.version_info[0])
if PY2:
    import ConfigParser
else:
    import configparser as ConfigParser
    raw_input = input
import subprocess

logging.basicConfig(level=logging.DEBUG, format='%(levelname)s:%(message)s')
_logger = logging.getLogger()
_this_dir = os.path.abspath(os.path.dirname(__file__))

settings = {
    "config_base": ".bumpversion.cfg",
    "start_dir": os.path.join(_this_dir, ".."),
    "ci_version": None,
    "commit_msg": None,
    "available_parts": ("none", "major", "minor", "patch", "push_tag"),
}


def check_output(cmd_arr):
    """ Simulate for python2.6. """
    proc = subprocess.Popen(cmd_arr, stdout=subprocess.PIPE)
    ret = proc.communicate()[0]
    return ret


class git(object):

    @staticmethod
    def commit_version():
        """
          Nice git version with commiter
        """
        project_version = check_output(
            "git log -1 --date=iso".split() +
            ["--pretty=format:%h %ad by %an"]
        )
        return project_version

    @staticmethod
    def commit(files, msg):
        cmd = "git add"
        check_output(cmd.split() + files)
        cmd = "git status"
        check_output(cmd.split())
        cmd = 'git commit -m'
        check_output(cmd.split() + [msg])

    @staticmethod
    def tag(version_str):
        cmd = "git tag v%s" % version_str
        check_output(cmd.split())

    @staticmethod
    def current_branch():
        """
          Nice git version with commiter
        """
        branch = check_output(
            "git rev-parse --abbrev-ref HEAD".split()
        ).strip()
        return branch


class bump_config(object):
    """
      Read bumpversion like config
    """

    def __init__(self, env):
        self.start_dir = env["start_dir"]
        self.config_base = env["config_base"]
        self.config_file = os.path.join(self.start_dir, self.config_base)
        self.config = ConfigParser.RawConfigParser()
        if not os.path.exists(self.config_file):
            _logger.critical(
                "Input configuration [%s] does not exist!", self.config_file)
            sys.exit(1)
        self.config.read(self.config_file)

        self.git_version = git.commit_version()
        self.ci_version = env["ci_version"]

        self.version = self.config.get("bumpversion", "current_version")
        self.new_version = self._bump_version(self.version, env["bump_part"])

        self.commit = env.get(
            "commit", self.config.getboolean("bumpversion", "commit")
        )
        self._tag = self.config.getboolean("bumpversion", "tag")

        self.regexp = None
        if env.get("replace_with", False) and self.config.has_option("mazoea", "parse"):
            self.regexp = self.config.get("mazoea", "parse")
            if self.config.has_option("mazoea", "commit"):
                self.commit = self.config.getboolean("mazoea", "commit")
                _logger.info("Using mazoea:commit [%s]", self.commit)

    def _bump_version(self, ver, part):
        major, minor, patch = ver.split(".")
        if "none" == part:
            pass
        elif "patch" == part:
            patch = int(patch) + 1
        elif "minor" == part:
            patch = 0
            minor = int(minor) + 1
        elif "major" == part:
            patch = 0
            minor = 0
            major = int(major) + 1
        return "%s.%s.%s" % (major, minor, patch)

    def __repr__(self):
        return u"<git_version:%s ci_version:%s>" % (
            self.git_version, self.ci_version
        )

    def version_files(self):
        prefix = "bumpversion:file:"
        arr = []
        for s in self.config.sections():
            if s.startswith(prefix):
                arr.append(os.path.join(self.start_dir, s[len(prefix):]))
        return arr

    def releasenotes_file(self):
        prefix = "bumpversion:releasenotes:"
        for s in self.config.sections():
            if s.startswith(prefix):
                return os.path.join(self.start_dir, s[len(prefix):])
        return None

    def bump(self, commit_msg=None):
        _logger.info(
            "Changing semver version from [%s] to [%s]%s",
            self.version,
            self.new_version,
            "" if commit_msg is None else " - " + commit_msg
        )

        other_files = []
        releasenotes = self.releasenotes_file()
        if releasenotes is not None and env["bump_part"] != "none":
            other_files.append(releasenotes)
            _logger.critical(
                "Have you updated release notes [%s]?!\n.....?", releasenotes
            )
            raw_input("")

        for version_file in self.version_files():
            if not os.path.exists(version_file):
                _logger.critical("File [%s] does not exist", version_file)
                sys.exit(1)

            with codecs.open(version_file, mode="rb", encoding="utf-8") as fin:
                lines = fin.readlines()
            lines = self._process_file(lines)
            with io.open(version_file, mode="w+", encoding="utf-8", newline="\n") as fout:
                # remove CRLF in case it got here somehow
                fout.writelines([x.rstrip() + "\n" for x in lines])

        self._finalise(msg_suffix=commit_msg, other_files=other_files)

    def _process_file(self, lines):

        # locals
        git_version = self.git_version
        ci_version = "" if self.ci_version is None \
            else " at %s" % self.ci_version

        # special replacing?
        r = None
        replace_with = None
        if self.regexp:
            r = re.compile(self.regexp)
            replace_with = self.config.get("mazoea", "replace_with")
            replace_with = replace_with.format(**locals())

        # should change version
        change_version = self.version != self.new_version
        # replaced
        replaced_version = False
        replaced_replace_with = False

        for i, line in enumerate(lines):
            newl = line

            if change_version:
                newl = newl.replace(self.version, self.new_version)
                if newl != line:
                    replaced_version = True

            if r is not None:
                newl2 = r.sub(replace_with, newl)
                if newl2 != newl:
                    replaced_replace_with = True
                    newl = newl2

            if newl != line:
                _logger.critical(
                    "Changing [\n%s\n] to [\n%s\n]", line.strip(), newl.strip())
                lines[i] = newl

        assert1 = not change_version or (change_version and replaced_version)
        assert2 = not r or (r and replaced_replace_with)
        if assert1 and assert2:
            return lines

        _logger.critical(
            "No replacements using [%s] and version [%s]!",
            self.regexp,
            self.version
        )
        sys.exit(1)

    def _finalise(self, msg_suffix=None, other_files=None):
        # update current version
        self.config.set("bumpversion", "current_version", self.new_version)
        self.config.write(open(self.config_file, "w+"))
        # commit if required
        if self.commit:
            _logger.info("Committing")
            msg = "bump version: %s -> %s" % (self.version, self.new_version)
            if msg_suffix is not None:
                msg += " " + msg_suffix
            files = [self.config_base] + self.version_files() + \
                other_files or []
            git.commit(files, msg)
            if self._tag and self.version != self.new_version:
                git.tag(self.new_version)

    def tag(self):
        branch = git.current_branch()
        date_tag = time.strftime("%Y-%m-%d@%H.%M")
        tag = "%s-%s-%s" % (branch, self.new_version, date_tag)
        git.tag(tag)
        return tag


# =====================================

def parse_command_line(env):
    """ Parses the command line arguments. """
    opts = None
    args = sys.argv[1:]
    bump_part = None
    for arg in args:
        if not arg.startswith("--") and arg in env["available_parts"]:
            bump_part = arg
            break

    if bump_part is None:
        _logger.critical("Which part to bump?")
        sys.exit(1)
    else:
        env["bump_part"] = bump_part
    args.remove(bump_part)

    try:
        options = [
            "ci_version=",
            "commit=",
            "replace_with=",
            "msg=",
        ]
        opts, args = getopt.getopt(args, "", options)
    except getopt.GetoptError as e:
        _logger.info(u"Invalid arguments [%s]", e)
        sys.exit(1)

    for option, param in opts:
        if option == "--ci_version":
            env["ci_version"] = param
        elif option == "--commit":
            env["commit"] = (param.lower() == "true")
        elif option == "--replace_with":
            env["replace_with"] = (param.lower() == "true")
        elif option == "--msg":
            env["commit_msg"] = param

    return env

# =====================================

# bumpversion can be used to
# - increase `major`/`minor`/`patch` semver numbering
# - do specific replacing `none`
# - tag with a `push_tag` that is semver + number
#
if __name__ == '__main__':
    os.putenv("TERM", "msys")

    env = parse_command_line(settings)
    bumper = bump_config(env)
    _logger.critical(bumper)
    if "push_tag" == env["bump_part"]:
        new_tag = bumper.tag()
        _logger.info("Tagged with [%s]", new_tag)
    else:
        bumper.bump(env["commit_msg"])
