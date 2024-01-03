describe('Navigator', () => {
  it('hardwareConcurrency', () => {
    expect(navigator.hardwareConcurrency > 0).toBeTrue();
  });

  it('platform', async () => {
    expect(navigator.platform).toBeDefined();
  });

  it('appName', () => {
    expect(navigator.appName).toBeDefined();
  });

  it('appVersion', () => {
    expect(navigator.appVersion).toBeDefined();
  });

  it('language', async () => {
    console.log(navigator.languages);
    expect(navigator.language).toBeDefined();
  });

  it('languages', async () => {
    expect(navigator.languages instanceof Array).toBeTrue();
    expect(navigator.languages[0]).toBeDefined();
  });

  it('userAgent', () => {
    expect(navigator.userAgent).toMatch(/WebF/);
  });

  it('clipboard', async () => {
    const text = String(new Date());
    await navigator.clipboard.writeText(text);
    const data = await navigator.clipboard.readText();
    expect(data).toBe(text);
  });
});
