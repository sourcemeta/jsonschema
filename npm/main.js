const os = require('os');
const path = require('path');
const fs = require('fs/promises');
const child_process = require('child_process');

const PLATFORM = os.platform() === 'win32' ? 'windows' : os.platform();
const ARCH = os.arch() === 'x64' ? 'x86_64' : os.arch();
const EXTENSION = PLATFORM === 'windows' ? '.exe' : '';
const EXECUTABLE = path.join(__dirname, '..', 'build', 'github-releases',
  `jsonschema-${PLATFORM}-${ARCH}${EXTENSION}`);

function spawnProcess(executable, args, options) {
  return new Promise((resolve, reject) => {
    const process = child_process.spawn(executable, args, options);

    let stdout = '';
    let stderr = '';

    if (process.stdout) {
      process.stdout.on('data', (data) => {
        stdout += data.toString();
      });
    }

    if (process.stderr) {
      process.stderr.on('data', (data) => {
        stderr += data.toString();
      });
    }

    process.on('error', (error) => {
      reject(error);
    });

    process.on('close', (code) => {
      resolve({ code, stdout, stderr });
    });
  });
}

async function spawn(args, options = {}) {
  const json = options.json === true;
  const spawnArgs = json ? [...args, '--json'] : args;

  try {
    await fs.access(EXECUTABLE);
  } catch {
    throw new Error(
      `The JSON Schema CLI NPM package does not support ${os.platform()} for ${os.arch()} yet. ` +
      'Please open a GitHub issue at https://github.com/sourcemeta/jsonschema'
    );
  }

  if (PLATFORM === 'darwin') {
    await spawnProcess('/usr/bin/xattr', ['-c', EXECUTABLE], { stdio: 'inherit' });
  }

  const spawnOptions = {
    windowsHide: true,
    ...options
  };

  delete spawnOptions.json;

  const result = await spawnProcess(EXECUTABLE, spawnArgs, spawnOptions);

  return {
    code: result.code,
    stdout: json ? JSON.parse(result.stdout) : result.stdout,
    stderr: result.stderr
  };
}

module.exports = { spawn };
