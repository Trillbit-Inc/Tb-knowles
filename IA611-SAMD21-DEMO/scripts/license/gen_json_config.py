import json
import base64

def gen_config(lic, ck, ck_nonce, outfilepath):
    cfg = {
        'license': lic,
        'ck': ck,
        'ck_nonce': ck_nonce
    }

    if outfilepath:
        with open(outfilepath, 'w') as f:
            json.dump(cfg, f)
    
    return base64.b64encode(json.dumps(cfg).encode('utf-8'))
