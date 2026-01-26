#!/usr/bin/env node

const { spawn } = require('./main.js');

spawn(process.argv.slice(2), { stdio: 'inherit' })
  .then((result) => {
    process.exit(result.code);
  })
  .catch((error) => {
    console.error(error.message);
    process.exit(1);
  });
