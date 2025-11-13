const addon = require('bindings')('tokenizer_addon');
module.exports = {
    createParserManager: addon.createParserManager,
    releaseParserManager: addon.releaseParserManager,
    getAllVisibleTokens: (pm, source, x0=0, y0=0, x1=9999, y1=9999) =>
        addon.getAllVisibleTokens(pm, source, x0, y0, x1, y1),
    getLanguages: addon.getLanguages
};
