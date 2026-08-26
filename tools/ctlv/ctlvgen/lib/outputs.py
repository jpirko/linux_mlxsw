# SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
"""The complete set of generated files and its reconciliation."""

from __future__ import annotations

import dataclasses
import os
import pathlib
import subprocess
import tempfile

from . import gen_doc
from . import gen_kernel
from . import gen_uapi
from . import gen_user
from . import semantics
from . import spec
from . import yamlload
from .errors import CtlvError


FLAVOURS = {
    'uapi-header': gen_uapi.render,
    'kernel-header': gen_kernel.render_header,
    'kernel-source': gen_kernel.render_source,
    'user-header': gen_user.render_header,
    'user-source': gen_user.render_source,
    'doc-rst': gen_doc.render,
}

IDENTITIES = {
    'uapi-header': ('uapi', 'header', '.h'),
    'kernel-header': ('kernel', 'header', '.h'),
    'kernel-source': ('kernel', 'source', '.c'),
    'user-header': ('user', 'header', '.h'),
    'user-source': ('user', 'source', '.c'),
    'doc-rst': ('doc', 'rst', '.rst'),
}

ROOT_FLAVOURS = frozenset(FLAVOURS) - {'user-header'}
FAMILY_FLAVOURS = frozenset(FLAVOURS)
FIXTURE_FLAVOURS = frozenset({
    'uapi-header', 'kernel-header', 'kernel-source',
})


@dataclasses.dataclass(frozen=True)
class Output:
    family: str
    scope: str
    spec: pathlib.PurePosixPath
    flavour: str
    path: pathlib.PurePosixPath


def _spec_paths(paths, source, scope):
    registry = paths.registry if scope == 'registered' else None
    return spec.Paths(paths.kernel_dir / source, schema=paths.schema,
                      registry=registry)


def _relative(where, value):
    path = pathlib.PurePosixPath(value)
    if path.is_absolute() or '..' in path.parts or \
            any(part.startswith('.') for part in path.parts) or \
            str(path) != value:
        raise CtlvError(where, f'"{value}" is not a normalized relative path')
    return path


def _family_names(records, scope):
    return [record['name'] for record in records if record['scope'] == scope]


def load(paths):
    """Load and validate the generated-file manifest."""
    data = yamlload.load(paths.generated)
    spec._validate_syntax(paths.generated, data, paths.generated_schema)
    records = data['families']
    registry = spec.load_registry(paths)
    registered = [entry.name for entry in registry.entries]
    fixtures_dir = paths.kernel_dir / 'tools/ctlv/tests/specs'
    fixtures = sorted(path.stem for path in fixtures_dir.glob('*.yaml'))

    if _family_names(records, 'registered') != registered:
        raise CtlvError(str(paths.generated),
                        'registered manifest families do not match the '
                        'ordered family registry')
    if _family_names(records, 'fixture') != fixtures:
        raise CtlvError(str(paths.generated),
                        'fixture manifest families do not match test specs')

    outputs = []
    families = set()
    destinations = set()
    for record in records:
        name = record['name']
        where = f'{paths.generated}: family {name}'
        if name in families:
            raise CtlvError(where, 'duplicate family')
        families.add(name)

        expected_spec = (
            pathlib.PurePosixPath('Documentation/ctlv/specs') /
            f'{name}.yaml'
            if record['scope'] == 'registered' else
            pathlib.PurePosixPath('tools/ctlv/tests/specs') /
            f'{name}.yaml'
        )
        source = _relative(where, record['spec'])
        if source != expected_spec:
            raise CtlvError(where, f'spec is {source}, expected '
                                   f'{expected_spec}')

        required = FIXTURE_FLAVOURS
        if record['scope'] == 'registered':
            required = ROOT_FLAVOURS if name == spec.CORE_FAMILY_NAME \
                else FAMILY_FLAVOURS
        flavours = frozenset(record['outputs'])
        if flavours != required:
            missing = sorted(required - flavours)
            extra = sorted(flavours - required)
            detail = []
            if missing:
                detail.append(f'missing {", ".join(missing)}')
            if extra:
                detail.append(f'unexpected {", ".join(extra)}')
            raise CtlvError(where, '; '.join(detail))

        family = semantics.load(_spec_paths(paths, source, record['scope']))
        if family.name != name:
            raise CtlvError(where, f'spec declares family {family.name}')
        for flavour, value in record['outputs'].items():
            destination = _relative(f'{where} {flavour}', value)
            suffix = IDENTITIES[flavour][2]
            if destination.suffix != suffix:
                raise CtlvError(f'{where} {flavour}',
                                f'output must end in {suffix}')
            if destination in destinations:
                raise CtlvError(f'{where} {flavour}',
                                f'duplicate output path {destination}')
            destinations.add(destination)
            outputs.append(Output(name, record['scope'], source, flavour,
                                  destination))
    return tuple(outputs)


def render(paths, plan):
    """Render every planned output before any destination is changed."""
    families = {}
    rendered = {}
    for output in plan:
        family = families.get(output.family)
        if family is None:
            family = semantics.load(_spec_paths(paths, output.spec,
                                                output.scope))
            families[output.family] = family
        renderer = FLAVOURS[output.flavour]
        if output.flavour in ('kernel-header', 'kernel-source'):
            text = renderer(family, family_api=output.scope == 'registered')
        else:
            text = renderer(family)
        mode, kind, _ = IDENTITIES[output.flavour]
        if _identity(text) != (output.spec.as_posix(), mode, kind):
            raise CtlvError(str(output.path),
                            'generator emitted the wrong output marker')
        rendered[output.path] = text
    return rendered


def _marked_files(root):
    if (root / '.git').exists():
        result = subprocess.run(
            ['git', 'grep', '--untracked', '--files-with-matches',
             'CTLV-GEN', '--', '*.c', '*.h', '*.rst'],
            cwd=root, capture_output=True, text=True, check=False)
        if result.returncode not in (0, 1):
            raise CtlvError(str(root), result.stderr.strip())
        candidates = (root / line for line in result.stdout.splitlines())
    else:
        candidates = (path for suffix in ('*.c', '*.h', '*.rst')
                      for path in root.rglob(suffix))

    marked = set()
    for path in candidates:
        if path.is_symlink() or not path.is_file():
            continue
        text = path.read_text(encoding='utf-8', errors='replace')
        if _identity(text) is not None:
            marked.add(path.relative_to(root).as_posix())
    return marked


def _destination(root, relative):
    destination = root.joinpath(*relative.parts)
    if destination.is_symlink():
        raise CtlvError(str(destination),
                        'generated output cannot be a symlink')
    parent = destination.parent
    while parent != root and not parent.exists():
        parent = parent.parent
    if parent.is_symlink() or root.resolve() not in (
            parent.resolve(), *parent.resolve().parents):
        raise CtlvError(str(destination),
                        'generated output escapes its output root')
    return destination


def _replace(destination, text):
    destination.parent.mkdir(parents=True, exist_ok=True)
    fd, temporary = tempfile.mkstemp(dir=destination.parent,
                                     prefix=f'.{destination.name}.')
    try:
        with os.fdopen(fd, 'w', encoding='utf-8') as stream:
            stream.write(text)
        os.chmod(temporary, 0o644)
        os.replace(temporary, destination)
    finally:
        if os.path.exists(temporary):
            os.unlink(temporary)


def _identity(text):
    lines = text.splitlines()
    if len(lines) < 4:
        return None
    c_spdx = (lines[0].startswith('// SPDX-License-Identifier: ') or
              (lines[0].startswith('/* SPDX-License-Identifier: ') and
               lines[0].endswith(' */')))
    if c_spdx and \
            lines[1] == ('/* Do not edit, regenerate with '
                         'tools/ctlv/ctlv-regen.sh, from: */') and \
            lines[2].startswith('/*\t') and lines[2].endswith(' */') and \
            lines[3].startswith('/* CTLV-GEN ') and lines[3].endswith(' */'):
        source = lines[2][3:-3]
        fields = lines[3][3:-3].split()
    elif lines[0].startswith('.. SPDX-License-Identifier: ') and \
            lines[1] == ('.. Do not edit, regenerate with '
                         'tools/ctlv/ctlv-regen.sh, from:') and \
            lines[2].startswith('..\t') and \
            lines[3].startswith('.. CTLV-GEN '):
        source = lines[2][3:]
        fields = lines[3][3:].split()
    else:
        return None
    if len(fields) == 3 and fields[0] == 'CTLV-GEN':
        return source, fields[1], fields[2]
    return None


def reconcile(paths, output_root=None, check=False):
    """Check or atomically update every generated output."""
    plan = load(paths)
    rendered = render(paths, plan)
    root = pathlib.Path(paths.kernel_dir if output_root is None
                        else output_root)
    if not root.is_dir():
        raise CtlvError(str(root), 'output root is not a directory')
    expected = {output.path.as_posix() for output in plan}
    unexpected = sorted(_marked_files(root) - expected)
    if unexpected:
        raise CtlvError(str(paths.generated),
                        'unexpected generated outputs\n' +
                        '\n'.join(f'  {path}' for path in unexpected))

    states = {}
    problems = []
    unsafe = []
    by_path = {output.path: output for output in plan}
    for relative, text in rendered.items():
        destination = _destination(root, relative)
        if not destination.exists():
            problems.append(f'missing {relative}')
            states[relative] = (destination, text)
            continue
        if not destination.is_file():
            raise CtlvError(str(destination),
                            'generated output is not a regular file')
        current = destination.read_text(encoding='utf-8')
        if current == text:
            continue
        problems.append(f'stale {relative}')
        output = by_path[relative]
        mode, kind, _ = IDENTITIES[output.flavour]
        if not check and _identity(current) != (
                output.spec.as_posix(), mode, kind):
            unsafe.append(f'{relative} is not the planned generated file')
        states[relative] = (destination, text)
    if unsafe:
        raise CtlvError('generated outputs',
                        'refusing to overwrite files\n' +
                        '\n'.join(f'  {problem}' for problem in unsafe))
    if check and problems:
        raise CtlvError('generated outputs', '\n'.join(problems))
    for destination, text in states.values():
        _replace(destination, text)
    return tuple(problems)
