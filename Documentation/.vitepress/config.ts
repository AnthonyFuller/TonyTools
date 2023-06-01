import { defineConfig } from "vitepress"
import { SitemapStream } from 'sitemap'
import { createWriteStream } from 'node:fs'
import { resolve } from 'node:path'

const links: any = []

export default defineConfig({
    title: "TonyTools",
    description: "Documentation hub for the TonyTools suite of libraries and tools.",
    themeConfig: {
        sidebar: [
            {
                text: "General",
                items: [
                    { text: "Installation", link: "/general/installation" },
                    { text: "Third-Party", link: "/general/thirdparty" }
                ]
            },
            {
                text: "Libraries",
                items: [
                    { text: "HMLanguages", link: "/libraries/hmlanguages" }
                ]
            },
            {
                text: "Tools",
                items: [
                    { text: "HMTextureTools", link: "/tools/hmtexturetools" },
                    { text: "HMLanguageTools", link: "/tools/hmlanguagetools" },
                    { text: "HMAScrambler", link: "/tools/hmascrambler" },
                    //{ text: "BOXCExporter", link: "/tools/boxcexporter" },
                    //{ text: "JSONPatchCreator", link: "/tools/jsonpatchcreator" },
                    //{ text: "MJBATesting", link: "/tools/mjbatesting" },
                    //{ text: "SCDA", link: "/tools/scda" },
                    //{ text: "VTXD", link: "/tools/vtxd" }
                ]
            }
        ],
        socialLinks: [
            { icon: "github", link: "https://github.com/AnthonyFuller/TonyTools" }
        ]
    },
    cleanUrls: true,
    // Sitemap setup
    lastUpdated: true,
    transformHtml: (_, id, { pageData }) => {
        if (!/[\\/]404\.html$/.test(id))
            links.push({
                url: pageData.relativePath.replace(/((^|\/)index)?\.md$/, '$2'),
                lastmod: pageData.lastUpdated
            })
    },
    buildEnd: async ({ outDir }) => {
        const sitemap = new SitemapStream({
            hostname: 'https://tonytools.win'
        })
        const writeStream = createWriteStream(resolve(outDir, 'sitemap.xml'))
        sitemap.pipe(writeStream)
        links.forEach((link) => sitemap.write(link))
        sitemap.end()
        await new Promise((r) => writeStream.on('finish', r))
    }
})
