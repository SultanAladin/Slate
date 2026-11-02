"""Check/apply the source-only SolidArc change to the requested Frontier branch; never commit, push or merge."""
from pathlib import Path
import argparse
import hashlib
import json
import subprocess

Directory = Path(__file__).resolve().parent


def Git(Target, *Arguments):
    return subprocess.check_output(['git', '-C', str(Target), *Arguments], text=True).strip()


def Main():
    Parser = argparse.ArgumentParser(description=__doc__)
    Parser.add_argument('--target', type=Path, required=True)
    Parser.add_argument('--apply', action='store_true', help='write the checked change; default is a dry run')
    Arguments = Parser.parse_args()
    Target = Arguments.target.resolve()
    Specification = json.loads((Directory / 'SourceManifest.json').read_text())
    Patch = Directory / Specification['patch']
    if hashlib.sha256(Patch.read_bytes()).hexdigest() != Specification['patch_sha256']:
        raise RuntimeError('Integration patch checksum differs')
    if Path(Git(Target, 'rev-parse', '--show-toplevel')).resolve() != Target:
        raise RuntimeError('--target must be the Frontier repository root')
    if Git(Target, 'branch', '--show-current') != Specification['target_branch']:
        raise RuntimeError('Select the requested Frontier branch before applying')
    subprocess.run(['git', '-C', str(Target), 'merge-base', '--is-ancestor',
                    Specification['target_commit'], 'HEAD'], check=True)
    if Git(Target, 'status', '--porcelain', '--untracked-files=no'):
        raise RuntimeError('Preserve existing tracked edits before applying')
    for Relative, Entry in Specification['files'].items():
        Pathname = Target / Relative
        if Entry['before_sha256'] is None:
            if Pathname.exists() or Pathname.is_symlink():
                raise RuntimeError('New CAD path already exists: ' + Relative)
        elif not Pathname.is_file() or Pathname.is_symlink() or \
                hashlib.sha256(Pathname.read_bytes()).hexdigest() != Entry['before_sha256']:
            raise RuntimeError('Frontier changed in a touched path; re-port rather than overwrite: ' + Relative)
    subprocess.run(['git', '-C', str(Target), 'apply', '--check', '--whitespace=nowarn', str(Patch)], check=True)
    print('PASS branch, ancestry, clean tracked tree, source preconditions and patch applicability')
    if not Arguments.apply:
        print('Dry run only; add --apply to write the SolidArc change')
        return
    subprocess.run(['git', '-C', str(Target), 'apply', '--whitespace=nowarn', str(Patch)], check=True)
    for Relative, Entry in Specification['files'].items():
        if hashlib.sha256((Target / Relative).read_bytes()).hexdigest() != Entry['after_sha256']:
            raise RuntimeError('Applied source checksum differs: ' + Relative)
    print('PASS applied source hashes. Changes are uncommitted; no branch or remote was moved.')


if __name__ == '__main__':
    Main()
