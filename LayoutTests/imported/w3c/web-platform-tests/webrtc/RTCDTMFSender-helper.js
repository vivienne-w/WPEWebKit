'use strict';

// Test is based on the following editor draft:
// https://w3c.github.io/webrtc-pc/archives/20170605/webrtc.html

// Code using this helper should also include RTCPeerConnection-helper.js
// in the main HTML file

// The following helper functions are called from RTCPeerConnection-helper.js:
//   getTrackFromUserMedia
//   doSignalingHandshake

// Create a RTCDTMFSender using getUserMedia()
// Connect the PeerConnection to another PC and wait until it is
// properly connected, so that DTMF can be sent.
function createDtmfSender(pc = new RTCPeerConnection()) {
  let dtmfSender;
  return getTrackFromUserMedia('audio')
  .then(([track, mediaStream]) => {
    const sender = pc.addTrack(track, mediaStream);
    dtmfSender = sender.dtmf;
    assert_true(dtmfSender instanceof RTCDTMFSender,
                'Expect audio sender.dtmf to be set to a RTCDTMFSender');
    // Note: spec bug open - https://github.com/w3c/webrtc-pc/issues/1774
    // on whether sending should be possible before negotiation.
    const pc2 = new RTCPeerConnection();
    Object.defineProperty(pc, 'otherPc', { value: pc2 });
    exchangeIceCandidates(pc, pc2);
    return doSignalingHandshake(pc, pc2);
  }).then(() => {
    if (!('canInsertDTMF' in dtmfSender)) {
      return Promise.resolve();
    }
    // Wait until dtmfSender.canInsertDTMF becomes true.
    // Up to 150 ms has been observed in test. Wait 1 second
    // in steps of 10 ms.
    // Note: Using a short timeout and rejected promise in order to
    // make test return a clear error message on failure.
    return new Promise((resolve, reject) => {
      let counter = 0;
      step_timeout(function checkCanInsertDTMF() {
        if (dtmfSender.canInsertDTMF) {
          resolve();
        } else {
          if (counter >= 100) {
            reject('Waited too long for canInsertDTMF');
            return;
          }
          ++counter;
          step_timeout(checkCanInsertDTMF, 10);
        }
      }, 0);
    });
  }).then(() => {
    return dtmfSender;
  });
}

/*
  Create an RTCDTMFSender and test tonechange events on it.
    testFunc
      Test function that is going to manipulate the DTMFSender.
      It will be called with:
        t - the test object
        sender - the created RTCDTMFSender
        pc - the associated RTCPeerConnection as second argument.
    toneChanges
      Array of expected tonechange events fired. The elements
      are array of 3 items:
        expectedTone
          The expected character in event.tone
        expectedToneBuffer
          The expected new value of dtmfSender.toneBuffer
        expectedDuration
          The rough time since beginning or last tonechange event
          was fired.
    desc
      Test description.
 */
function test_tone_change_events(testFunc, toneChanges, desc) {
  promise_test(t => {
    let done, fail;
    const promise = new Promise((resolve, reject) => {
        done = resolve;
        fail = reject;
    });
    const pc = new RTCPeerConnection();

    createDtmfSender(pc)
    .then(dtmfSender => {
      let lastEventTime = Date.now();

      const onToneChange = t.step_func(ev => {
        assert_true(ev instanceof RTCDTMFToneChangeEvent,
          'Expect tone change event object to be an RTCDTMFToneChangeEvent');

        const { tone } = ev;
        assert_equals(typeof tone, 'string',
          'Expect event.tone to be the tone string');

        assert_greater_than(toneChanges.length, 0,
          'More tonechange event is fired than expected');

        const [
          expectedTone, expectedToneBuffer, expectedDuration
        ] = toneChanges.shift();

        assert_equals(tone, expectedTone,
          `Expect current event.tone to be ${expectedTone}`);

        assert_equals(dtmfSender.toneBuffer, expectedToneBuffer,
          `Expect dtmfSender.toneBuffer to be updated to ${expectedToneBuffer}`);

        const now = Date.now();
        const duration = now - lastEventTime;

        // We check that the delay is at least the expected one, but
        // system load may cause random delay, so we do not put any
        // realistic upper bound on the timing of the event.
        assert_between_inclusive(duration, expectedDuration,
                                 expectedDuration + 4000,
          `Expect tonechange event for "${tone}" to be fired approximately after ${expectedDuration} milliseconds`);

        lastEventTime = now;

        if (toneChanges.length === 0) {
          // Wait for same duration as last expected duration + 100ms
          // before passing test in case there are new tone events fired,
          // in which case the test should fail.
          t.step_timeout(
            t.step_func(() => {
              done();
              pc.close();
              pc.otherPc.close();
            }), expectedDuration + 100);
        }
      });

      dtmfSender.addEventListener('tonechange', onToneChange);
      testFunc(t, dtmfSender, pc);
    })
    .catch(t.step_func(err => {
      assert_unreached(`Unexpected promise rejection: ${err}`);
    }));
    return promise;
  }, desc);
}

// Get the one and only tranceiver from pc.getTransceivers().
// Assumes that there is only one tranceiver in pc.
function getTransceiver(pc) {
  const transceivers = pc.getTransceivers();
  assert_equals(transceivers.length, 1,
    'Expect there to be only one tranceiver in pc');

  return transceivers[0];
}
