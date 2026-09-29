import {cpSync,writeFileSync} from 'node:fs';
const raw=process.env.PROCESSPAY_BACKEND_URL;
if(!raw)throw new Error('Set PROCESSPAY_BACKEND_URL to the deployed HTTPS Linux backend before building.');
const url=new URL(raw);
if(url.protocol!=='https:' || url.username || url.password || url.pathname!=='/' || url.search || url.hash)throw new Error('Backend must be an HTTPS origin without credentials, path, query or fragment.');
cpSync('dist','netlify-dist',{recursive:true});
writeFileSync('netlify-dist/config.js',`window.PROCESSPAY_BACKEND = ${JSON.stringify(url.origin)};\n`);
console.log('Netlify frontend built; real Linux backend configured.');
