import json
import unittest
from pathlib import Path
from unittest.mock import patch

repo_root = Path(__file__).resolve().parent.parent
import importlib.machinery
import importlib.util

loader = importlib.machinery.SourceFileLoader("omarchy_minimize", str(repo_root / "scripts" / "omarchy-minimize"))
spec = importlib.util.spec_from_loader("omarchy_minimize", loader)
omarchy_minimize = importlib.util.module_from_spec(spec)
loader.exec_module(omarchy_minimize)


class TestMinimizeSecurity(unittest.TestCase):
    def test_clamp_text_bounds_length(self):
        """Ensure very long titles are strictly clamped to max_len."""
        long_title = "A" * 10000
        clamped = omarchy_minimize.clamp_text(long_title, 128)
        self.assertEqual(len(clamped), 128)

    def test_clamp_text_preserves_literal_markup(self):
        """Ensure HTML/XML markup like <b>controlled</b> remains literal without corruption."""
        markup_title = "<b>controlled</b> & \"quotes\""
        clamped = omarchy_minimize.clamp_text(markup_title, 128)
        self.assertEqual(clamped, "<b>controlled</b> & \"quotes\"")

    def test_clamp_text_strips_control_characters(self):
        """Ensure newlines and control characters are stripped."""
        dirty = "Line1\nLine2\r\t\x00End"
        clamped = omarchy_minimize.clamp_text(dirty, 128)
        self.assertNotIn("\n", clamped)
        self.assertNotIn("\r", clamped)
        self.assertNotIn("\x00", clamped)

    def test_status_output_bounded(self):
        """Ensure cmd_status produces clamped titles and classes."""
        fake_state = [
            {
                "address": "0x1234",
                "class": "C" * 500,
                "title": "<b>controlled</b>" + ("T" * 5000),
                "workspace": {"name": "W" * 200},
                "offscreen_at": [50000, 50000],
            }
        ]

        with patch.object(omarchy_minimize, "reconcile_state", return_value=(fake_state, [])):
            with patch("sys.stdout") as mock_stdout:
                omarchy_minimize.cmd_status(as_json=True)

        # Test status JSON output
        # Re-run capturing json
        with patch.object(omarchy_minimize, "reconcile_state", return_value=(fake_state, [])):
            import io
            buf = io.StringIO()
            with patch("sys.stdout", buf):
                omarchy_minimize.cmd_status(as_json=True)
            data = json.loads(buf.getvalue())

            group = data["groups"][0]
            self.assertLessEqual(len(group["class"]), 64)
            win = group["windows"][0]
            self.assertLessEqual(len(win["title"]), 128)
            self.assertTrue(win["title"].startswith("<b>controlled</b>"))
            self.assertLessEqual(len(win["workspace"]), 32)


if __name__ == "__main__":
    unittest.main()
